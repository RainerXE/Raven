/**
 * Raven - functions/_lib/receipt.js
 * Builds the "acknowledgement of receipt" PDF sent with a personal (lifetime) license.
 * No dependency: the PDF is written by hand (A4, standard fonts), so it runs in a Cloudflare Worker.
 *
 * Hidden signature: every receipt carries, twice (PDF metadata "Keywords" + an invisible line of text
 * at the bottom of the page), a payload signed with HMAC-SHA256:
 *
 *     raven-receipt-1.<license id>.<name b64url>.<email b64url>.<issued>.<type>.<hmac hex>
 *
 * The HMAC key is RAVEN_RECEIPT_SECRET (Cloudflare secret). Without it nobody can forge a receipt;
 * `python raven_receipt.py --verify receipt.pdf` checks one (same secret in the environment).
 */

const PAGE_W = 595.28;
const PAGE_H = 841.89;
const MX = 62.36; /* left margin (22 mm) */
const MR = PAGE_W - 62.36; /* right edge of the content */
const GRAY = [0.45, 0.5, 0.5];
const BAND = [0.82, 0.82, 0.82];
const GREEN = [0.2, 0.55, 0.5];
const BLACK = [0, 0, 0];

/* Helvetica advance widths (1/1000 em) for ASCII 32..126, to right-align the small header/footer text */
const HELV = [278, 278, 355, 556, 556, 889, 667, 191, 333, 333, 389, 584, 278, 333, 278, 278, 556, 556, 556, 556, 556, 556, 556, 556, 556, 556, 278, 278, 584, 584, 584, 556, 1015, 667, 667, 722, 722, 667, 611, 778, 722, 278, 500, 667, 556, 833, 722, 778, 667, 778, 722, 667, 611, 722, 667, 944, 667, 667, 611, 278, 278, 278, 469, 556, 333, 556, 556, 500, 556, 556, 278, 556, 556, 222, 222, 500, 222, 833, 556, 556, 556, 556, 333, 500, 278, 556, 500, 722, 500, 500, 500, 334, 260, 334, 584];

function helvWidth(text, size) {
	let w = 0;
	for (const ch of text) {
		const code = ch.charCodeAt(0);
		w += (code >= 32 && code <= 126 ? HELV[code - 32] : 556) * size / 1000;
	}
	return w;
}

/* ---- text helpers ---- */

/* Standard PDF fonts only know Latin-1: keep what fits, drop accents otherwise, "?" for the rest. */
function latin1(value) {
	let out = "";
	for (const ch of String(value).normalize("NFC")) {
		const code = ch.charCodeAt(0);
		if (ch.length === 1 && ((code >= 32 && code <= 126) || code >= 160) && code <= 255) { out += ch; continue; }
		const folded = ch.normalize("NFKD").replace(/[\u0300-\u036f]/g, "");
		out += folded.length === 1 && folded.charCodeAt(0) >= 32 && folded.charCodeAt(0) <= 126 ? folded : "?";
	}
	return out;
}

function esc(text) {
	return latin1(text).replace(/[\\()]/g, (c) => "\\" + c);
}

function b64url(text) {
	const bytes = new TextEncoder().encode(String(text));
	let s = "";
	for (const b of bytes) s += String.fromCharCode(b);
	return btoa(s).replace(/\+/g, "-").replace(/\//g, "_").replace(/=+$/, "");
}

function toHex(bytes) {
	return Array.from(bytes, (b) => b.toString(16).padStart(2, "0")).join("");
}

export async function signReceipt({ id, name, email, issued, type, secret }) {
	if (!secret) throw new Error("RAVEN_RECEIPT_SECRET is not set");
	const message = ["raven-receipt-1", id, b64url(name), b64url(email), issued, type].join(".");
	const key = await crypto.subtle.importKey("raw", new TextEncoder().encode(secret), { name: "HMAC", hash: "SHA-256" }, false, ["sign"]);
	const mac = new Uint8Array(await crypto.subtle.sign("HMAC", key, new TextEncoder().encode(message)));
	return message + "." + toHex(mac);
}

/* ---- tiny PDF drawing layer ---- */

function num(n) { return (Math.round(n * 100) / 100).toString(); }

class Page {
	constructor() { this.ops = []; }
	fill(c) { this.ops.push(c.map(num).join(" ") + " rg"); }
	stroke(c) { this.ops.push(c.map(num).join(" ") + " RG"); }
	line(x1, y1, x2, y2, width = 0.6) { this.ops.push(num(width) + " w " + num(x1) + " " + num(y1) + " m " + num(x2) + " " + num(y2) + " l S"); }
	rect(x, y, w, h) { this.ops.push(num(x) + " " + num(y) + " " + num(w) + " " + num(h) + " re f"); }
	circle(cx, cy, r) {
		const k = 0.5523 * r;
		this.ops.push(
			num(cx + r) + " " + num(cy) + " m " +
			num(cx + r) + " " + num(cy + k) + " " + num(cx + k) + " " + num(cy + r) + " " + num(cx) + " " + num(cy + r) + " c " +
			num(cx - k) + " " + num(cy + r) + " " + num(cx - r) + " " + num(cy + k) + " " + num(cx - r) + " " + num(cy) + " c " +
			num(cx - r) + " " + num(cy - k) + " " + num(cx - k) + " " + num(cy - r) + " " + num(cx) + " " + num(cy - r) + " c " +
			num(cx + k) + " " + num(cy - r) + " " + num(cx + r) + " " + num(cy - k) + " " + num(cx + r) + " " + num(cy) + " c f"
		);
	}
	/* font: F1 Helvetica, F2 Helvetica-Bold, F3 Courier, F4 Courier-Bold */
	text(x, y, str, font, size, { spacing = 0, invisible = false } = {}) {
		this.ops.push(
			"BT /" + font + " " + num(size) + " Tf " + num(spacing) + " Tc " + (invisible ? "3" : "0") + " Tr " +
			num(x) + " " + num(y) + " Td (" + esc(str) + ") Tj ET"
		);
	}
}

const MONO_W = 0.6; /* Courier: every character is 0.6 em wide */

/* The size at which a Courier text still fits in maxWidth */
function fit(text, maxWidth, size) {
	return Math.min(size, maxWidth / (MONO_W * Math.max(1, latin1(text).length)));
}

function monoField(page, y, label, value, boldValue = true) {
	const size = 11, dotsTo = 300;
	const lw = (label.length + 1) * MONO_W * size;
	page.fill(BLACK);
	page.text(MX, y, label + " ", "F3", size);
	page.text(MX + lw, y, ".".repeat(Math.max(2, Math.floor((dotsTo - MX - lw) / (MONO_W * size)))), "F3", size);
	page.text(dotsTo + 6, y, value, boldValue ? "F4" : "F3", fit(value, MR - dotsTo - 6, size));
}

/**
 * Returns the receipt as a Uint8Array (PDF).
 *   name, email: the licensee      licenseId: 32 hex digits of the .raven-license
 *   orderId: the Polar order id    issued: Date      secret: RAVEN_RECEIPT_SECRET
 */
export async function makeReceiptPdf({ name, email, licenseId, orderId = "", issued = new Date(), secret, type = "personal" }) {
	const issuedIso = issued.toISOString().slice(0, 19) + "Z";
	const day = issuedIso.slice(0, 10), clock = issuedIso.slice(11, 16) + " UTC";
	const receiptNo = "RV-" + licenseId.slice(0, 8).toUpperCase();
	const payload = await signReceipt({ id: licenseId, name, email, issued: issuedIso, type, secret });

	const p = new Page();
	const H = PAGE_H;

	/* header */
	p.fill(GRAY);
	p.text(MX, H - 107.7, "RAVEN", "F2", 34, { spacing: 3 });
	p.fill(GREEN);
	p.circle(MX + 12, H - 107.7 + 40, 2.2);
	p.stroke(GRAY);
	p.line(MX, H - 121.9, MR, H - 121.9);
	p.fill(GRAY);
	["GUI library for C", "Software license", "License service"].forEach((l, i) => p.text(MX, H - 147.4 - i * 10, l, "F1", 7.5));
	["github.com/oscar-soirey/Raven", "Contact: oscarsoirey.contact@gmail.com"].forEach((l, i) =>
		p.text(MR - helvWidth(latin1(l), 7.5), H - 147.4 - i * 10, l, "F1", 7.5));
	p.line(MX, H - 187.1, MR, H - 187.1);

	/* identification block */
	let y = H - 255.15;
	p.fill(BLACK);
	p.text(MX, y, "Raven License Service", "F3", 12);
	const licensee = "Licensee " + name.toUpperCase();
	p.text(PAGE_W / 2 + 10, y, licensee, "F4", fit(licensee, MR - PAGE_W / 2 - 10, 12));
	y -= 30;
	p.text(MX, y, "Receipt no. " + receiptNo, "F3", 12); y -= 16;
	p.text(MX, y, "Issued on   " + day + " at " + clock, "F3", 12); y -= 16;
	if (orderId) { const o = "Order ref.  " + orderId; p.text(MX, y, o, "F3", fit(o, MR - MX, 12)); y -= 16; }
	p.text(MX, y, "PURCHASE DATE", "F3", 10);
	p.text(MX + 125, y, day, "F3", 12); y -= 16;
	const mail = "Licensee e-mail : " + email;
	p.text(MX, y, mail, "F3", fit(mail, MR - MX, 12)); y -= 16;
	p.text(MX, y, "Full edition", "F3", 12);
	p.text(MR - 40 - 7 * MONO_W * 12, y, "Page  1", "F3", 12);

	/* section band */
	y -= 55;
	const bw = 250, spacing = 1.2, band = "S O F T W A R E   L I C E N S E";
	p.fill(BAND);
	p.rect(PAGE_W / 2 - bw / 2, y - 5, bw, 17);
	p.fill(BLACK);
	p.text(PAGE_W / 2 - (band.length * (MONO_W * 10 + spacing) - spacing) / 2, y, band, "F4", 10, { spacing });

	/* body */
	y -= 50;
	p.text(MX, y, "ACKNOWLEDGEMENT OF RECEIPT", "F4", 11);
	y -= 24; monoField(p, y, "Software", "RAVEN");
	y -= 15; monoField(p, y, "License type", "PERSONAL - LIFETIME");
	y -= 15; monoField(p, y, "Licensee", name.toUpperCase());
	y -= 15; monoField(p, y, "E-mail", email, false);
	y -= 28;
	p.text(MX, y, "Personal Raven license: valid for life, for every future version.", "F3", 11);
	y -= 32;
	[["LICENSE ID", licenseId], ["LICENSE FILE", "raven.raven-license"], ["LICENSE STATUS", "VALID"], ["PAYMENT", "RECEIVED"]].forEach(([k, v], i) => {
		p.text(MX, y - i * 15, k, "F4", 11);
		p.text(MX + 175, y - i * 15, "..: " + v, "F4", 11);
	});

	/* notes */
	p.fill(GRAY);
	p.text(MX, 114, "Your license file (raven.raven-license) is sent with this receipt by e-mail.", "F3", 8);
	p.text(MX, 102, "This document is your proof of purchase. Please keep it safe.", "F3", 8);
	p.text(MX, 90, "Lost it, or any question? oscarsoirey.contact@gmail.com", "F3", 8);

	/* footer */
	p.stroke(GRAY);
	p.line(MX, 68, MR, 68);
	p.fill(GREEN);
	p.circle(MX + 12, 45, 9);
	p.fill(BLACK);
	p.text(MX + 28, 38, "RAVEN", "F2", 22);
	p.fill(GRAY);
	p.text(MX + 150, 50, "Raven licenses", "F1", 7.5);
	p.text(MX + 150, 40, "Contact: oscarsoirey.contact@gmail.com", "F1", 7.5);
	const ref = "Ref. " + receiptNo, iss = "Issued " + day;
	p.text(MR - helvWidth(ref, 7.5), 50, ref, "F1", 7.5);
	p.text(MR - helvWidth(iss, 7.5), 40, iss, "F1", 7.5);

	/* hidden signature, copy 2: an invisible line of text (copy 1 is in the metadata) */
	p.fill(BLACK);
	p.text(MX, 6, payload, "F1", 1, { invisible: true });

	return buildPdf(p.ops.join("\n"), {
		Title: "Raven license - acknowledgement of receipt " + receiptNo,
		Author: "Raven",
		Creator: "Raven license service",
		Keywords: payload,
		CreationDate: "D:" + issuedIso.replace(/[-:T]/g, "").slice(0, 14) + "Z"
	});
}

function buildPdf(content, info) {
	const fonts = ["Helvetica", "Helvetica-Bold", "Courier", "Courier-Bold"];
	const objects = [
		"<< /Type /Catalog /Pages 2 0 R >>",
		"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
		"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 " + PAGE_W + " " + PAGE_H + "] /Resources << /Font << /F1 5 0 R /F2 6 0 R /F3 7 0 R /F4 8 0 R >> >> /Contents 4 0 R >>",
		"<< /Length " + content.length + " >>\nstream\n" + content + "\nendstream",
		...fonts.map((f) => "<< /Type /Font /Subtype /Type1 /BaseFont /" + f + " /Encoding /WinAnsiEncoding >>"),
		"<< " + Object.entries(info).map(([k, v]) => "/" + k + " (" + esc(v) + ")").join(" ") + " >>"
	];
	let out = "%PDF-1.4\n";
	const offsets = [];
	objects.forEach((body, i) => {
		offsets.push(out.length);
		out += (i + 1) + " 0 obj\n" + body + "\nendobj\n";
	});
	const xref = out.length;
	out += "xref\n0 " + (objects.length + 1) + "\n0000000000 65535 f \n";
	for (const off of offsets) out += String(off).padStart(10, "0") + " 00000 n \n";
	out += "trailer\n<< /Size " + (objects.length + 1) + " /Root 1 0 R /Info " + objects.length + " 0 R >>\nstartxref\n" + xref + "\n%%EOF\n";

	/* every character is < 256 (see latin1), so one char = one byte */
	const bytes = new Uint8Array(out.length);
	for (let i = 0; i < out.length; i++) bytes[i] = out.charCodeAt(i) & 255;
	return bytes;
}
