# Raven license delivery (Polar → Cloudflare Worker → email)

```
buyer clicks "Buy" on docs/license.html
  → Polar checkout (buy.polar.sh/...)            payment, taxes, invoice
  → Polar redirects to docs/thanks.html
  → Polar webhook "order.paid" → this Worker     checks the Polar signature
       → signs a .raven-license (same format as `raven_license issue`)
       → emails it to the buyer (resend.com), a copy to you
```

- **Personal ($20 once)**: `type: personal`, `expires: never`.
- **Company ($200/month)**: every paid month (first payment and each renewal
  are an `order.paid`) sends a new license valid until the end of that month
  + 5 days. Cancelled subscription → no new license → the last one expires.
- An order is served once (KV `LICENSES`), even when Polar retries.
- The license is never put into the buyer's program; it is used by CMake
  (`-DRAVEN_LICENSE_FILE=...`) to unlock the Release edition.

## Setup, once (about 20 minutes)

### 1. Keys

If you have not done it: `raven_license keygen %USERPROFILE%\.raven-license`,
copy `public.key` to `tools/license/public.key`, build the packages with
`dist.bat` (they are encrypted with your `unlock.key`). Back up
`signing.secret` and `unlock.key`.

### 2. Resend (the email)

1. Create an account on https://resend.com (free: 3 000 emails/month).
2. **Domains → Add domain**: a domain you own, add the DNS records it shows.
   (Without a domain, Resend only sends to your own address: fine to test, not to sell.)
3. **API Keys → Create** (permission "Sending access") → keep the `re_...` key.

### 3. Cloudflare Worker

```bat
cd tools\license\polar-worker
npm install
npx wrangler login
npx wrangler kv namespace create LICENSES
```

Paste the KV id in `wrangler.toml`, plus `MAIL_FROM` (the Resend domain).
Then the secrets (paste the content of each file, one line of hex):

```bat
npx wrangler secret put RAVEN_SIGNING_SECRET     & rem content of signing.secret
npx wrangler secret put RAVEN_UNLOCK_KEY         & rem content of unlock.key
npx wrangler secret put RESEND_API_KEY           & rem re_...
npx wrangler deploy
```

`deploy` prints the URL, e.g. `https://raven-license.<you>.workers.dev`.
Opening it shows `ok`.

### 4. Polar

1. **Products**: open each product, copy its id, put them in `wrangler.toml`
   (`PRODUCT_PERSONAL`, `PRODUCT_COMPANY`). Remove the "License keys" benefit
   if you added one (the Worker replaces it).
2. **Checkout links**: success URL `https://oscar-soirey.github.io/Raven/thanks.html`
   (the links are already in `docs/license.html`).
3. **Settings → Webhooks → Add endpoint**:
   URL `https://raven-license.<you>.workers.dev/polar/webhook`, format **Raw**,
   event **order.paid**. Copy its secret (`whsec_...`):
   ```bat
   npx wrangler secret put POLAR_WEBHOOK_SECRET
   npx wrangler deploy
   ```

### 5. Test in the sandbox

Do steps 3–4 once more on https://sandbox.polar.sh (its own products and
webhook secret; a second Worker `npx wrangler deploy --name raven-license-sandbox`
is the simplest), buy with the card `4242 4242 4242 4242`, and check:

- the email arrives with `raven.raven-license`;
- `raven_license show raven.raven-license` says `signature: valid`;
- `npx wrangler tail` shows `sent personal license ...`;
- Polar → Webhooks → the delivery is green (200). "Redeliver" does not send a second email.

## Daily use

Nothing to do. To see what was sent: the copies in your mailbox (`MAIL_BCC`),
`npx wrangler tail`, or `npx wrangler kv key list --binding LICENSES`.
A buyer lost the file: `raven_license issue` by hand, as before.

## Tests

`npm test` (Node 20+). With real keys, also checked by the C tool:
`node test/test.mjs <keys folder>` then `raven_license show test-personal.raven-license --keys <keys folder>`.
