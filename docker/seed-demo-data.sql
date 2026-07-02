-- Demo data for local development.
-- Player names are lowercase on purpose: the view pages bind data via
-- data-player="mikko" etc., and gamelogger.js matches the DB player name
-- exactly (case-sensitive). Run any time to refresh "now"-relative data:
--   docker compose exec -T db mariadb -ugamelogger -pgamelogger gamelogger \
--     < docker/seed-demo-data.sql

SET @now    = UNIX_TIMESTAMP();
SET @began  = @now - 3600;          -- current session started ~1h ago
SET @m_began = @now - 180;          -- minute buckets: last 3 minutes
SET @past_began = @now - 86400;     -- a finished game from ~yesterday
SET @past_end   = @now - 82800;

-- Clean previous demo rows so re-running stays idempotent.
DELETE FROM gl_playing     WHERE player IN ('mikko','juha','sami','pavel','tero','ville');
DELETE FROM gl_apm         WHERE player IN ('mikko','juha','sami','pavel','tero','ville');
DELETE FROM gl_apm_minute  WHERE player IN ('mikko','juha','sami','pavel','tero','ville');
DELETE FROM gl_apm_history WHERE player IN ('mikko','juha','sami','pavel','tero','ville');
DELETE FROM gl_history     WHERE player IN ('mikko','juha','sami','pavel','tero','ville');

-- Active sessions (drives currentlyplaying.php + the live gauges).
-- gameid values exist in settings.txt.
INSERT INTO gl_playing (player, gameid, began, updated, apmsum, updates) VALUES
  ('mikko',  1, @began, @now, 45000, 600),   -- AI War,        avg ~75
  ('juha',  10, @began, @now, 36000, 600),   -- Left 4 Dead 2, avg ~60
  ('sami',  23, @began, @now, 54000, 600),   -- Civ V,         avg ~90
  ('pavel', 40, @began, @now, 30000, 600),   -- Diablo 3,      avg ~50
  ('tero',  14, @began, @now, 48000, 600),   -- Terraria,      avg ~80
  ('ville', 51, @began, @now, 39000, 600);   -- FTL,           avg ~65

-- Latest APM buffer (drives observeAPM: histogram + monitor.html "ok").
INSERT INTO gl_apm (player, updated, apm) VALUES
  ('mikko', @now, '72,78,75,80,70'),
  ('juha',  @now, '58,62,60,65,55'),
  ('sami',  @now, '88,92,90,95,85'),
  ('pavel', @now, '48,52,50,55,45'),
  ('tero',  @now, '78,82,80,85,75'),
  ('ville', @now, '63,67,65,70,60');

-- Per-minute APM aggregation (drives observeHistory apm series).
INSERT INTO gl_apm_minute (player, began, ends, apm) VALUES
  ('mikko', @m_began,      @m_began+60,  '70,72,74,76,78'),
  ('mikko', @m_began+60,   @m_began+120, '75,73,71,77,79'),
  ('juha',  @m_began,      @m_began+60,  '58,60,62,59,61'),
  ('sami',  @m_began,      @m_began+60,  '90,88,92,91,89'),
  ('pavel', @m_began,      @m_began+60,  '50,48,52,49,51'),
  ('tero',  @m_began,      @m_began+60,  '80,78,82,79,81'),
  ('ville', @m_began,      @m_began+60,  '65,63,67,64,66');

-- Hi-res APM history (drives observeHiResAPMHistory).
INSERT INTO gl_apm_history (player, updated, apm) VALUES
  ('mikko', @now-60, '70,72,75,73,71'),
  ('mikko', @now,    '72,78,75,80,70'),
  ('juha',  @now,    '58,62,60,65,55'),
  ('sami',  @now,    '88,92,90,95,85'),
  ('pavel', @now,    '48,52,50,55,45'),
  ('tero',  @now,    '78,82,80,85,75'),
  ('ville', @now,    '63,67,65,70,60');

-- A finished game per player (drives the gamelogger.php history timeline).
INSERT INTO gl_history (player, gameid, began, updated, apmsum, updates) VALUES
  ('mikko',  3, @past_began, @past_end, 30000, 500),  -- Civ 4
  ('juha',  16, @past_began, @past_end, 24000, 500),  -- Company of Heroes
  ('sami',  33, @past_began, @past_end, 40000, 500),  -- Civ 5
  ('pavel', 19, @past_began, @past_end, 20000, 500),  -- Magicka
  ('tero',  43, @past_began, @past_end, 35000, 500),  -- Spelunky
  ('ville', 25, @past_began, @past_end, 28000, 500);  -- Alien Swarm
