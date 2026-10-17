/* Cloudflare Pages Function: POST /polar/webhook (Polar "order.paid" -> license by email).
   The code is in ../_lib/worker.js; the settings are in Cloudflare (Settings > Variables and Secrets). */
import worker from "../_lib/worker.js";

export async function onRequestPost({ request, env }) {
	const url = new URL(request.url);
	url.pathname = "/polar/webhook";
	return worker.fetch(new Request(url, request), env);
}
