-- Blind Sight backend schema.
-- Design note: the game server is the only writer of progression. Nothing here
-- is reachable with a player token; see routes/server.ts.

CREATE TABLE IF NOT EXISTS players (
    player_id            UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    platform             TEXT NOT NULL,                 -- 'eos' | 'steam' | 'dev'
    platform_account_id  TEXT NOT NULL,
    display_name         TEXT NOT NULL DEFAULT 'Hider',
    level                INTEGER NOT NULL DEFAULT 1,
    experience           INTEGER NOT NULL DEFAULT 0,
    currency             INTEGER NOT NULL DEFAULT 0 CHECK (currency >= 0),
    matches_played       INTEGER NOT NULL DEFAULT 0,
    rounds_as_hunter     INTEGER NOT NULL DEFAULT 0,
    total_catches        INTEGER NOT NULL DEFAULT 0,
    total_survivals      INTEGER NOT NULL DEFAULT 0,
    equipped_hunter_skin TEXT NOT NULL DEFAULT 'default',
    equipped_hider_skin  TEXT NOT NULL DEFAULT 'default',
    banned               BOOLEAN NOT NULL DEFAULT FALSE,
    created_at           TIMESTAMPTZ NOT NULL DEFAULT now(),
    last_seen_at         TIMESTAMPTZ NOT NULL DEFAULT now(),
    UNIQUE (platform, platform_account_id)
);

CREATE TABLE IF NOT EXISTS cosmetics (
    cosmetic_id  TEXT PRIMARY KEY,
    slot         TEXT NOT NULL,              -- 'hunter_skin' | 'hider_skin' | 'throwable'
    display_name TEXT NOT NULL,
    price        INTEGER NOT NULL CHECK (price >= 0),
    enabled      BOOLEAN NOT NULL DEFAULT TRUE
);

CREATE TABLE IF NOT EXISTS player_cosmetics (
    player_id   UUID REFERENCES players(player_id) ON DELETE CASCADE,
    cosmetic_id TEXT REFERENCES cosmetics(cosmetic_id),
    acquired_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    PRIMARY KEY (player_id, cosmetic_id)
);

CREATE TABLE IF NOT EXISTS game_servers (
    server_id     UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    region        TEXT NOT NULL,
    address       TEXT NOT NULL,
    build         TEXT,
    state         TEXT NOT NULL DEFAULT 'idle',   -- idle | lobby | in_progress | finished
    player_count  INTEGER NOT NULL DEFAULT 0,
    current_match UUID,
    last_heartbeat TIMESTAMPTZ NOT NULL DEFAULT now(),
    created_at    TIMESTAMPTZ NOT NULL DEFAULT now()
);
CREATE INDEX IF NOT EXISTS idx_servers_alloc ON game_servers (region, state, last_heartbeat);

CREATE TABLE IF NOT EXISTS matches (
    match_id   UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    mode       TEXT NOT NULL,
    region     TEXT NOT NULL,
    map_name   TEXT,
    server_id  UUID REFERENCES game_servers(server_id),
    state      TEXT NOT NULL DEFAULT 'allocating',
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    ended_at   TIMESTAMPTZ
);

CREATE TABLE IF NOT EXISTS match_players (
    match_id  UUID REFERENCES matches(match_id) ON DELETE CASCADE,
    player_id UUID REFERENCES players(player_id) ON DELETE CASCADE,
    PRIMARY KEY (match_id, player_id)
);

CREATE TABLE IF NOT EXISTS match_rounds (
    id           BIGSERIAL PRIMARY KEY,
    match_id     UUID REFERENCES matches(match_id) ON DELETE CASCADE,
    round_number INTEGER NOT NULL,
    mode         TEXT NOT NULL,
    recorded_at  TIMESTAMPTZ NOT NULL DEFAULT now(),
    UNIQUE (match_id, round_number)       -- makes result submission idempotent
);

CREATE TABLE IF NOT EXISTS round_results (
    id             BIGSERIAL PRIMARY KEY,
    round_id       BIGINT REFERENCES match_rounds(id) ON DELETE CASCADE,
    player_id      UUID REFERENCES players(player_id) ON DELETE CASCADE,
    role           TEXT NOT NULL,
    points         INTEGER NOT NULL DEFAULT 0,
    placement      INTEGER NOT NULL DEFAULT 0,
    catches        INTEGER NOT NULL DEFAULT 0,
    survived       BOOLEAN NOT NULL DEFAULT FALSE,
    survival_secs  REAL NOT NULL DEFAULT 0,
    shots_fired    INTEGER NOT NULL DEFAULT 0,
    throwables_used INTEGER NOT NULL DEFAULT 0
);
CREATE INDEX IF NOT EXISTS idx_results_player ON round_results (player_id);

CREATE TABLE IF NOT EXISTS matchmaking_tickets (
    ticket_id  UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    player_id  UUID REFERENCES players(player_id) ON DELETE CASCADE,
    mode       TEXT NOT NULL,
    region     TEXT NOT NULL,
    party_id   TEXT,
    state      TEXT NOT NULL DEFAULT 'queued',   -- queued | matched | failed | cancelled
    match_id   UUID REFERENCES matches(match_id),
    created_at TIMESTAMPTZ NOT NULL DEFAULT now()
);
CREATE INDEX IF NOT EXISTS idx_tickets_queue ON matchmaking_tickets (state, mode, region, created_at);

-- Telemetry is append-only and deliberately schemaless in its payload, so designers
-- can add events during balance passes without a migration.
CREATE TABLE IF NOT EXISTS telemetry_events (
    id         BIGSERIAL PRIMARY KEY,
    event      TEXT NOT NULL,
    match_id   TEXT,
    player_id  TEXT,
    round_time DOUBLE PRECISION,
    numbers    JSONB NOT NULL DEFAULT '{}'::jsonb,
    strings    JSONB NOT NULL DEFAULT '{}'::jsonb,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now()
);
CREATE INDEX IF NOT EXISTS idx_telemetry_event_time ON telemetry_events (event, created_at);
CREATE INDEX IF NOT EXISTS idx_telemetry_match ON telemetry_events (match_id);

INSERT INTO cosmetics (cosmetic_id, slot, display_name, price) VALUES
    ('default',           'hunter_skin', 'Standard Issue',  0),
    ('hunter_butcher',    'hunter_skin', 'The Butcher',   1500),
    ('hunter_scarecrow',  'hunter_skin', 'Scarecrow',     2500),
    ('hider_default',     'hider_skin',  'Standard Issue',   0),
    ('hider_traffic_cone','hider_skin',  'Traffic Cone',   800),
    ('hider_mascot',      'hider_skin',  'Team Mascot',   2000)
ON CONFLICT (cosmetic_id) DO NOTHING;
