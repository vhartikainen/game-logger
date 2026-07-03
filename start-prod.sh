#!/usr/bin/env bash
#
# Start (or update) the GameLogger production stack.
#
# Pulls the latest images and brings up db + web + duckdns defined in
# docker-compose.prod.yml. Safe to re-run: it's the update path too.
#
# Usage:
#   ./start-prod.sh          # pull latest images and (re)start the stack
#   ./start-prod.sh down     # stop and remove the stack (keeps db volume)
#   ./start-prod.sh logs     # follow logs
#   ./start-prod.sh status   # show container status

set -euo pipefail

cd "$(dirname "$0")"

COMPOSE_FILE="docker-compose.prod.yml"

# Resolve the compose command (v2 plugin preferred, fall back to legacy binary).
if docker compose version >/dev/null 2>&1; then
    COMPOSE=(docker compose)
elif command -v docker-compose >/dev/null 2>&1; then
    COMPOSE=(docker-compose)
else
    echo "ERROR: neither 'docker compose' nor 'docker-compose' is available." >&2
    exit 1
fi

compose() { "${COMPOSE[@]}" -f "$COMPOSE_FILE" "$@"; }

case "${1:-up}" in
    down)
        compose down
        exit 0
        ;;
    logs)
        compose logs -f
        exit 0
        ;;
    status|ps)
        compose ps
        exit 0
        ;;
    up)
        ;; # fall through to the start logic below
    *)
        echo "Usage: $0 [up|down|logs|status]" >&2
        exit 1
        ;;
esac

# --- start / update path ---------------------------------------------------

if [[ ! -f .env ]]; then
    echo "ERROR: .env not found. Copy .env.example to .env and fill in secrets:" >&2
    echo "  cp .env.example .env" >&2
    echo "At minimum set DUCKDNS_SUBDOMAIN and DUCKDNS_TOKEN (no defaults)." >&2
    exit 1
fi

# Warn about DuckDNS values being empty (they have no compose defaults).
if ! grep -qE '^DUCKDNS_SUBDOMAIN=.+' .env || ! grep -qE '^DUCKDNS_TOKEN=.+' .env; then
    echo "WARNING: DUCKDNS_SUBDOMAIN and/or DUCKDNS_TOKEN look empty in .env." >&2
    echo "         The duckdns container will not update DNS without them." >&2
fi

echo ">> Pulling latest images..."
compose pull

echo ">> Starting stack..."
compose up -d

echo ">> Current status:"
compose ps
