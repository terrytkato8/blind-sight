import express from 'express';
import helmet from 'helmet';
import rateLimit from 'express-rate-limit';
import pinoHttp from 'pino-http';
import { config, devAuthMode } from './config.js';
import { pool } from './db.js';
import authRoutes from './routes/auth.js';
import profileRoutes from './routes/profile.js';
import shopRoutes from './routes/shop.js';
import matchmakingRoutes from './routes/matchmaking.js';
import serverRoutes from './routes/server.js';
import { runMatchmaker } from './matchmaker.js';

const app = express();
app.use(helmet());
app.use(express.json({ limit: '1mb' }));
app.use(pinoHttp({ autoLogging: { ignore: (req) => req.url === '/health' } }));

// Clients are rate limited; dedicated servers are not, since they post bursts of telemetry.
app.use(['/auth', '/profile', '/shop', '/matchmaking'],
  rateLimit({ windowMs: 60_000, limit: 120, standardHeaders: true, legacyHeaders: false }));

app.get('/health', async (_req, res) => {
  try {
    await pool.query('SELECT 1');
    res.json({ ok: true });
  } catch {
    res.status(503).json({ ok: false });
  }
});

app.use('/auth', authRoutes);
app.use('/profile', profileRoutes);
app.use('/shop', shopRoutes);
app.use('/matchmaking', matchmakingRoutes);
app.use('/server', serverRoutes);

app.use((_req, res) => res.status(404).json({ error: 'not_found' }));

const server = app.listen(config.port, () => {
  if (devAuthMode) {
    console.warn('\n*** DEV AUTH MODE: identity tokens are NOT verified. Do not expose this to the internet. ***\n');
  }
  if (!config.serverKey) {
    console.warn('*** SERVER_KEY is unset — /server routes are disabled, so no progression will persist. ***');
  }
  console.log(`Blind Sight backend listening on :${config.port}`);
});

// The matchmaker is a loop rather than a request handler: it wakes every two seconds,
// forms matches from queued tickets, and allocates an idle server to each.
const matchmakerTimer = setInterval(() => { runMatchmaker().catch((e) => console.error('matchmaker', e)); }, 2000);

for (const sig of ['SIGTERM', 'SIGINT'] as const) {
  process.on(sig, () => {
    clearInterval(matchmakerTimer);
    server.close(() => pool.end().then(() => process.exit(0)));
  });
}
