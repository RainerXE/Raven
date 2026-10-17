/**
 * Raven - tools/license/polar-worker/src/license.js
 * Makes a .raven-license exactly as `raven_license issue` does (license_format.h):
 *
 *     Raven license 1
 *     name: ...
 *     email: ...
 *     type: personal | company
 *     issued: YYYY-MM-DD
 *     expires: never | YYYY-MM-DD
 *     id: 32 hex digits
 *     unlock: 64 hex digits
 *     signature: 128 hex digits   (Ed25519 over every line above)
 *
 * Ed25519 is deterministic: WebCrypto and TweetNaCl give the same signature.
 */

export function toHex(bytes) {
	return Array.from(bytes, (b) => b.toString(16).padStart(2, "0")).join("");
}

export function fromHex(text, size) {
	const clean = String(text || "").trim().toLowerCase();
	if (!/^[0-9a-f]*$/.test(clean) || clean.length !== size * 2) throw new Error("bad key: expected " + size + " bytes of hex");
	const out = new Uint8Array(size);
	for (let i = 0; i < size; i++) out[i] = parseInt(clean.substr(i * 2, 2), 16);
	return out;
}

function base64url(bytes) {
	let s = "";
	for (const b of bytes) s += String.fromCharCode(b);
	return btoa(s).replace(/\+/g, "-").replace(/\//g, "_").replace(/=+$/, "");
}

/* A field of the license: one line, as the C tool accepts it (< 256 bytes, no line break). */
function field(value, what) {
	const text = String(value || "").replace(/[\r\n\t]+/g, " ").trim();
	if (!text) throw new Error("the license needs a " + what);
	const bytes = new TextEncoder().encode(text);
	if (bytes.length >= 256) throw new Error("the " + what + " is too long");
	return text;
}

export function isoDate(date) {
	return date.toISOString().slice(0, 10);
}

/**
 * signingSecretHex: the content of signing.secret (64 bytes: seed then public key)
 * unlockHex: the content of unlock.key (32 bytes)
 */
export async function makeLicense({ name, email, type, expires, signingSecretHex, unlockHex, today = new Date(), id = null }) {
	if (type !== "personal" && type !== "company") throw new Error("type is personal or company");
	if (expires !== "never" && !/^\d{4}-\d{2}-\d{2}$/.test(expires)) throw new Error("expires is YYYY-MM-DD or never");
	const sk = fromHex(signingSecretHex, 64);
	const unlock = fromHex(unlockHex, 32);
	const licenseId = id || crypto.getRandomValues(new Uint8Array(16));

	const body =
		"Raven license 1\n" +
		"name: " + field(name, "name") + "\n" +
		"email: " + field(email, "email") + "\n" +
		"type: " + type + "\n" +
		"issued: " + isoDate(today) + "\n" +
		"expires: " + expires + "\n" +
		"id: " + toHex(licenseId) + "\n" +
		"unlock: " + toHex(unlock) + "\n";

	const key = await crypto.subtle.importKey(
		"jwk",
		{ kty: "OKP", crv: "Ed25519", d: base64url(sk.slice(0, 32)), x: base64url(sk.slice(32, 64)) },
		{ name: "Ed25519" },
		false,
		["sign"]
	);
	const signature = new Uint8Array(await crypto.subtle.sign({ name: "Ed25519" }, key, new TextEncoder().encode(body)));
	return { text: body + "signature: " + toHex(signature) + "\n", id: toHex(licenseId) };
}
