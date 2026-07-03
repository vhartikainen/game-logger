# GameLogger backend — Docker setup

Runs the PHP server and a MariaDB database locally with one command. Aimed at
development on macOS (Apple Silicon included) and Linux.

## What's included

- **web** — `php:7.4-apache` serving `../server`. PHP 7.4 is used on purpose:
  the legacy code relies on short open tags (`<?`) and unquoted array keys,
  both fatal under PHP 8. The `mysqli` extension and `short_open_tag` are
  enabled in the image.
- **db** — `mariadb:10.6` (arm64-native; `mysql:5.7` has no official arm64
  image). The schema in `server/createTables.sql` is applied automatically the
  first time the database starts.

## Usage

```bash
# from the repo root
cp .env.example .env        # optional — defaults work out of the box
docker compose up -d --build
```

Then open the app:

- http://127.0.0.1:8090/gamelogger/clientConnect.php?request=settings

> Use `127.0.0.1`, not `localhost`. On macOS `localhost` resolves to IPv6
> (`::1`) first, which the Docker/OrbStack port proxy may reset.
>
> The host port defaults to **8090** (set `WEB_PORT` in `.env` to change it).
> 8080/8088 are commonly already taken by other local containers.

Stop / reset:

```bash
docker compose down          # stop containers, keep the database volume
docker compose down -v       # also wipe the database (schema re-applies next up)
```

## Configuration

Connection details are passed to PHP via environment variables, read in
`server/gamelogger/database.php` (`DB_HOST`, `DB_NAME`, `DB_USER`,
`DB_PASSWORD`). Override ports and credentials in `.env` — see `.env.example`.

The `./server` directory is bind-mounted into the web container, so edits to
PHP/JS/CSS are picked up on the next request — no rebuild needed.

## Production deployment

On push to `master` (or a `v*` tag), [`.github/workflows/docker-publish.yml`](../.github/workflows/docker-publish.yml)
builds the `web` image and pushes it to GitHub Container Registry as
`ghcr.io/vhartikainen/game-logger-web:latest`.

On the server:

```bash
git clone https://github.com/vhartikainen/game-logger.git
cd game-logger
cp .env.example .env        # edit DB credentials/ports as needed
docker compose -f docker-compose.prod.yml pull
docker compose -f docker-compose.prod.yml up -d
```

`docker-compose.prod.yml` pulls the pre-built image instead of building it and
does not bind-mount `./server` or expose the database port — only the web
port is published. To deploy an update, `git pull` (for the updated
`createTables.sql`/compose file) then re-run the `pull` and `up -d` commands
above.

The first time the workflow runs, the package it creates on GHCR is private
by default. Either make it public (Package settings → Change visibility) or
`docker login ghcr.io` on the server with a token that has `read:packages`
before pulling.

## Making it reachable over the internet (dynamic IP, no domain)

`docker-compose.prod.yml` includes a `duckdns` service that keeps a free
[DuckDNS](https://www.duckdns.org) hostname pointed at your server's current
public IP.

1. Sign in at duckdns.org and create a subdomain (e.g. `gamelogger` →
   `gamelogger.duckdns.org`). Copy the token shown on the account page.
2. Add both to `.env`:
   ```
   DUCKDNS_SUBDOMAIN=gamelogger
   DUCKDNS_TOKEN=<your token>
   ```
3. On your router, forward port 80 (the `WEB_PORT` mapping) to the server's
   local IP.
4. `docker compose -f docker-compose.prod.yml up -d` — the `duckdns`
   container checks and updates the DNS record every few minutes on its own.

The site is then reachable at `http://gamelogger.duckdns.org:8090` (or
whatever `WEB_PORT` is set to; use `80` to drop the port from the URL).

## Fixed on this branch

`server/gamelogger/browserConnect.php`'s `observe*`/`resetAPM` functions
referenced `$mysqli` without declaring `global $mysqli`, so the browser-facing
endpoints fataled with "Call to a member function query() on null". This was a
bug in the original source (the client endpoints pass `$mysqli` explicitly and
were unaffected) that surfaced on any PHP host; it has been corrected here.
