export const config = {
  port: Number(process.env.PORT ?? 8080),
  databaseUrl: process.env.DATABASE_URL ?? 'postgres://blindsight:blindsight@localhost:5432/blindsight',
  jwtSecret: process.env.JWT_SECRET ?? 'dev-only-insecure-secret',
  serverKey: process.env.SERVER_KEY ?? '',
  eos: {
    productId: process.env.EOS_PRODUCT_ID ?? '',
    clientId: process.env.EOS_CLIENT_ID ?? '',
    clientSecret: process.env.EOS_CLIENT_SECRET ?? '',
  },
  matchSize: Number(process.env.MATCH_SIZE ?? 6),
  matchMinSize: Number(process.env.MATCH_MIN_SIZE ?? 4),
};

/** True when no identity provider is configured — tokens are trusted blindly. Dev only. */
export const devAuthMode = !config.eos.clientId;
