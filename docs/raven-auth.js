/* raven-auth.js — charge Clerk (service d'authentification hébergé) et expose window.RavenAuth.
 * Aucune logique de mot de passe ici : Clerk gère inscription, connexion, vérification
 * d'email, réinitialisation, sessions. Mets ta clé publique (pk_test_… puis pk_live_…) ci-dessous. */
(function(){
	var PUBLISHABLE_KEY = "pk_test_REMPLACE_MOI";

	function load(src, attrs){
		return new Promise(function(resolve, reject){
			var s = document.createElement("script");
			s.src = src; s.async = true; s.crossOrigin = "anonymous";
			for (var k in attrs || {}) s.setAttribute(k, attrs[k]);
			s.onload = resolve;
			s.onerror = function(){ reject(new Error("Clerk failed to load")); };
			document.head.appendChild(s);
		});
	}

	/* résout avec l'instance Clerk, une fois chargée */
	var ready = Promise.resolve().then(function(){
		/* le domaine de l'API Clerk est encodé dans la clé publique */
		var base = "https://" + atob(PUBLISHABLE_KEY.split("_")[2]).slice(0, -1) + "/npm/";
		return load(base + "@clerk/ui@1/dist/ui.browser.js").then(function(){
			return load(base + "@clerk/clerk-js@6/dist/clerk.browser.js", { "data-clerk-publishable-key": PUBLISHABLE_KEY });
		}).then(function(){
			return window.Clerk.load({ ui: { ClerkUI: window.__internal_ClerkUICtor } });
		}).then(function(){ return window.Clerk; });
	});

	/* appel de notre API avec le jeton de session Clerk */
	function api(path, opts){
		opts = opts || {};
		return ready.then(function(clerk){
			return clerk.session ? clerk.session.getToken() : null;
		}).then(function(token){
			var h = {};
			if (token) h.Authorization = "Bearer " + token;
			return fetch("/api/" + path, { method: opts.method || "GET", headers: h, credentials: "same-origin" });
		}).then(function(r){
			return r.json().catch(function(){ return {}; }).then(function(j){ return { ok: r.ok, status: r.status, j: j }; });
		});
	}

	window.RavenAuth = { ready: ready, api: api };
})();
