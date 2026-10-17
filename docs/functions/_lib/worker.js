/**
 * Raven - tools/license/polar-worker/src/worker.js
 * Cloudflare Worker: Polar says "order paid" -> the buyer gets a signed .raven-license by email.
 *
 *   POST /polar/webhook   the endpoint given to Polar (event: order.paid)
 *   GET  /                "ok" (to check the deployment)
 *
 * Secrets (wrangler secret put ...):
 *   POLAR_WEBHOOK_SECRET   the whsec_... secret of the Polar webhook endpoint
 *   RAVEN_SIGNING_SECRET   the content of signing.secret
 *   RAVEN_UNLOCK_KEY       the content of unlock.key
 *   RESEND_API_KEY         the API key of resend.com
 * Variables (wrangler.toml [vars]):
 *   PRODUCT_PERSONAL       Polar product id of the $20 license
 *   PRODUCT_COMPANY        Polar product id of the $200/month license
 *   MAIL_FROM              "Raven <license@your-domain>" (a domain verified on Resend)
 *   MAIL_BCC               your address: a copy of every license
 *   COMPANY_GRACE_DAYS     days added after the paid month (default 5)
 * KV (optional, recommended): LICENSES — Polar retries a webhook; the same order never gets two licenses.
 */

import { makeLicense, isoDate } from "./license.js";

export default {
	async fetch(request, env) {
		const url = new URL(request.url);
		if (request.method === "GET" && url.pathname === "/") return new Response("ok");
		if (request.method !== "POST" || url.pathname !== "/polar/webhook") return new Response("not found", { status: 404 });

		const body = await request.text();
		if (!(await verifyWebhook(request.headers, body, env.POLAR_WEBHOOK_SECRET))) return new Response("bad signature", { status: 403 });

		let event;
		try { event = JSON.parse(body); } catch { return new Response("bad json", { status: 400 }); }
		if (event.type !== "order.paid") return new Response("ignored");

		try {
			const result = await handleOrder(event.data, env);
			return new Response(result);
		} catch (error) {
			/* 500: Polar retries later */
			console.error("license failed", error && error.stack ? error.stack : error);
			return new Response("error: " + (error && error.message), { status: 500 });
		}
	}
};

export async function handleOrder(order, env, send = sendMail) {
	const productId = order.product_id || (order.product && order.product.id);
	let type;
	if (productId && productId === env.PRODUCT_PERSONAL) type = "personal";
	else if (productId && productId === env.PRODUCT_COMPANY) type = "company";
	else return "ignored: product " + productId;

	if (env.LICENSES) {
		const done = await env.LICENSES.get("order:" + order.id);
		if (done) return "already sent: " + done;
	}

	const customer = order.customer || {};
	const email = customer.email || order.customer_email;
	const name = customer.name || (order.billing_name) || email;
	if (!email) throw new Error("the order has no customer email");

	let expires = "never";
	if (type === "company") {
		/* one license per paid month: valid until the end of that month, plus a few days to renew */
		const end = order.subscription && order.subscription.current_period_end
			? new Date(order.subscription.current_period_end)
			: new Date(Date.now() + 31 * 86400000);
		const grace = parseInt(env.COMPANY_GRACE_DAYS || "5", 10);
		expires = isoDate(new Date(end.getTime() + grace * 86400000));
	}

	const license = await makeLicense({
		name, email, type, expires,
		signingSecretHex: env.RAVEN_SIGNING_SECRET,
		unlockHex: env.RAVEN_UNLOCK_KEY
	});
	await send(env, { to: email, name, type, expires, license: license.text, id: license.id });
	if (env.LICENSES) await env.LICENSES.put("order:" + order.id, license.id + " " + email + " " + type + " " + expires);
	return "sent " + type + " license " + license.id + " to " + email;
}

/* ---- the email (resend.com) ---- */

async function sendMail(env, { to, name, type, expires, license, id }) {
	const renew = type === "company"
		? "This license is valid until " + expires + ". Each monthly payment sends you a new one automatically: replace the file when it arrives."
		: "This license never expires and works with every future version of Raven.";
	const text =
		"Hello " + name + ",\n\n" +
		"Thank you for buying a Raven license! Your license file is attached (raven.raven-license).\n\n" +
		"To build without the watermark:\n" +
		"  1. Download the Raven package: https://github.com/oscar-soirey/Raven/releases\n" +
		"  2. Keep raven.raven-license somewhere outside your project's repository.\n" +
		"  3. Configure your project with:\n" +
		"       cmake -B build -DRAVEN_LICENSE_FILE=C:/path/to/raven.raven-license\n" +
		"     find_package(raven) then links the Release edition. The license itself is never put into your program.\n\n" +
		renew + "\n\n" +
		"Keep this file to yourself: it is in your name.\n" +
		"License id: " + id + "\n\n" +
		"A question? Just reply to this email.\n";

	const response = await fetch("https://api.resend.com/emails", {
		method: "POST",
		headers: { "Authorization": "Bearer " + env.RESEND_API_KEY, "Content-Type": "application/json" },
		body: JSON.stringify({
			from: env.MAIL_FROM,
			to: [to],
			bcc: env.MAIL_BCC ? [env.MAIL_BCC] : undefined,
			reply_to: env.MAIL_BCC || undefined,
			subject: type === "company" ? "Your Raven company license (until " + expires + ")" : "Your Raven license",
			text,
			attachments: [{ filename: "raven.raven-license", content: btoa(unescape(encodeURIComponent(license))) }]
		})
	});
	if (!response.ok) throw new Error("resend " + response.status + ": " + (await response.text()));
}

/* ---- Polar webhook signature (Standard Webhooks) ---- */

export async function verifyWebhook(headers, body, secret, now = Date.now()) {
	const id = headers.get("webhook-id"), timestamp = headers.get("webhook-timestamp"), signatures = headers.get("webhook-signature");
	if (!id || !timestamp || !signatures || !secret) return false;
	if (Math.abs(now / 1000 - parseInt(timestamp, 10)) > 300) return false;

	/* Polar: endpoints made before 2026-09-08 sign with the UTF-8 bytes of the whole "whsec_..." string,
	   later ones with the base64 key after "whsec_" (Standard Webhooks). Both are tried. */
	const keys = [new TextEncoder().encode(secret)];
	if (secret.startsWith("whsec_")) {
		try { keys.push(Uint8Array.from(atob(secret.slice(6)), (c) => c.charCodeAt(0))); } catch { /* not base64 */ }
	}
	const message = new TextEncoder().encode(id + "." + timestamp + "." + body);
	const given = signatures.split(" ").map((s) => s.split(",")).filter((p) => p[0] === "v1").map((p) => p[1]);
	for (const raw of keys) {
		const key = await crypto.subtle.importKey("raw", raw, { name: "HMAC", hash: "SHA-256" }, false, ["sign"]);
		const mac = new Uint8Array(await crypto.subtle.sign("HMAC", key, message));
		let expected = "";
		for (const b of mac) expected += String.fromCharCode(b);
		expected = btoa(expected);
		if (given.some((g) => g === expected)) return true;
	}
	return false;
}
