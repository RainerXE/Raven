/* Raven - Worker entry: the webhook goes to the license code, everything else is the static site. */
import worker from "./_lib/worker.js";

export default {
	async fetch(request, env, ctx) {
		const url = new URL(request.url);
		if (url.pathname === "/polar/webhook") return worker.fetch(request, env);
		return env.ASSETS.fetch(request);
	}
};
