#!/usr/bin/env python3
"""Black-box contract and state-transition tests for the native BinGoat API."""
import concurrent.futures
import json
import os
import sys
import urllib.error
import urllib.request

BASE = os.environ.get("BINGOAT_TEST_URL", "http://127.0.0.1:18080")


def request(path, method="GET", payload=None, raw=None, content_type="application/json"):
    data = raw if raw is not None else (
        json.dumps(payload).encode("utf-8") if payload is not None else None
    )
    headers = {}
    if data is not None:
        headers["Content-Type"] = content_type
    req = urllib.request.Request(BASE + path, data=data, headers=headers, method=method)
    try:
        with urllib.request.urlopen(req, timeout=10) as response:
            body = response.read()
            return response.status, json.loads(body) if body else None
    except urllib.error.HTTPError as error:
        body = error.read()
        try:
            parsed = json.loads(body) if body else None
        except json.JSONDecodeError:
            parsed = body.decode("utf-8", "replace")
        return error.code, parsed


def expect(condition, message):
    if not condition:
        raise AssertionError(message)


def game():
    status, data = request("/api/games/current?boardCols=10")
    expect(status == 200, f"GET current: HTTP {status}")
    return data


def sync(count, names=None, regenerate=False):
    payload = {"playerCount": count, "regenerateCards": regenerate, "autoMark": True}
    if names is not None:
        payload["playerNames"] = names
    return request("/api/games/current/sync", "POST", payload)


def validate_cards(players):
    expect(2 <= len(players) <= 5, f"invalid player count: {len(players)}")
    for player in players:
        values = player["numbers"]
        expect(len(values) == 15, f"player {player['id']} does not have 15 numbers")
        expect(all(type(n) is int and 1 <= n <= 60 for n in values),
               f"player {player['id']} has a number outside 1..60")
        expect(len(set(values)) == 15, f"player {player['id']} has duplicate numbers")
        ordered = sorted(values)
        expect(all(b - a != 1 for a, b in zip(ordered, ordered[1:])),
               f"player {player['id']} has consecutive numbers")


def main():
    # Health, initial state and card invariants.
    status, health = request("/api/health")
    expect(status == 200 and health.get("status") == "ok" and health.get("engine") == "ready",
           "health check did not report a ready engine")
    initial = game()
    validate_cards(initial["players"])
    expect(len(initial["players"]) == 2 and initial["calledNumbers"] == [],
           "initial game must contain two players and no draws")

    # Player counts, names and invalid values.
    status, two = sync(2, ["Ana", "Bia"])
    expect(status == 200 and [p["name"] for p in two["players"]] == ["Ana", "Bia"],
           "two-player synchronization failed")
    validate_cards(two["players"])
    status, five = sync(5, ["Ana", "Bia", "Caio", "Duda", "Eva"], True)
    expect(status == 200 and len(five["players"]) == 5, "five-player game creation failed")
    validate_cards(five["players"])
    for invalid_count in (1, 6):
        status, _ = sync(invalid_count)
        expect(status == 400, f"playerCount={invalid_count} should return HTTP 400")
    for bad_payload in (
        {"playerCount": "5"},
        {"playerCount": 3, "playerNames": "Ana"},
        {"playerCount": 3, "playerNames": ["Ana", 42, "Caio"]},
        {"playerCount": 3, "playerNames": [""]},
    ):
        status, _ = request("/api/games/current/sync", "POST", bad_payload)
        expect(status == 400, f"invalid sync payload should return 400: {bad_payload}")
    status, _ = request("/api/games/current/sync", "POST",
                        {"playerCount": 2, "playerNames": ["x" * 33, "Bia"]})
    expect(status == 400, "player names longer than 32 characters must be rejected")

    # Malformed JSON, missing fields, wrong types, invalid ranges and missing players.
    for raw in (b"{", b'{"number": 10} trailing'):
        status, _ = request("/api/games/current/toggle-board", "POST", raw=raw)
        expect(status == 400, "malformed JSON must return HTTP 400")
    status, _ = request("/api/games/current/draw", "POST", payload={}, content_type="text/plain")
    expect(status == 415, "unsupported content type must return HTTP 415")
    for payload in ({"number": 0}, {"number": 61}, {"number": "10"}, {}):
        status, _ = request("/api/games/current/toggle-board", "POST", payload)
        expect(status == 400, f"invalid toggle-board payload should return 400: {payload}")
    status, _ = request("/api/games/current/toggle-card", "POST",
                        {"playerId": 99, "number": 1})
    expect(status == 409, "unknown player should return HTTP 409")
    status, _ = request("/api/games/current", "POST", payload={})
    expect(status == 405, "unsupported method should return HTTP 405")
    status, _ = request("/api/games/current?boardCols=7")
    expect(status == 400, "unsupported boardCols should return HTTP 400")

    # Reset, concurrent draws, then verify state consistency and no duplicate numbers.
    status, reset = request("/api/games/current/reset", "POST", {"newCards": True})
    expect(status == 200 and reset["calledNumbers"] == [], "reset failed")
    def draw(_):
        return request("/api/games/current/draw", "POST", {"boardCols": 10, "autoMark": True})
    with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:
        results = list(pool.map(draw, range(8)))
    expect(all(code == 200 for code, _ in results),
           f"concurrent draws returned unexpected statuses: {[x[0] for x in results]}")
    concurrent_numbers = [body["drawnNumber"] for _, body in results]
    expect(len(set(concurrent_numbers)) == len(concurrent_numbers),
           "concurrent draws returned duplicate numbers")
    snapshot = game()
    expect(len(snapshot["calledNumbers"]) == len(set(snapshot["calledNumbers"])),
           "calledNumbers contains duplicates")
    expect(set(concurrent_numbers).issubset(set(snapshot["calledNumbers"])),
           "concurrent draw results are missing from authoritative state")
    expect(all(1 <= n <= 60 for n in snapshot["calledNumbers"]),
           "drawn number outside 1..60")
    validate_cards(snapshot["players"])

    # Manual marking, undo, and reset state.
    status, marked = request("/api/games/current/toggle-board", "POST",
                             {"number": 60, "autoMark": False})
    expect(status == 200 and 60 in marked["calledNumbers"], "manual board marking failed")
    player_number = snapshot["players"][0]["numbers"][0]
    status, card = request("/api/games/current/toggle-card", "POST",
                           {"playerId": 1, "number": player_number})
    expect(status == 200 and player_number in card["players"][0]["markedNumbers"],
           "manual card marking failed")
    status, undone = request("/api/games/current/undo", "POST", {})
    expect(status == 200, "undo failed")
    status, cleared = request("/api/games/current/reset", "POST", {"newCards": False})
    expect(status == 200 and cleared["calledNumbers"] == [], "reset did not clear history")

    print("API contract, card invariants, validation, concurrency and state transitions: PASS")


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        print(f"API contract tests FAILED: {exc}", file=sys.stderr)
        sys.exit(1)
