"use strict";
(async function(){
  try {
    const r = await fetch("/api/me", { credentials:"same-origin", cache:"no-store" });
    if (!r.ok) return;
    const d = await r.json();
    if (!d.authenticated) return;

    const nav = document.querySelector("[data-topnav]");
    const actions = document.querySelector("[data-auth-actions]");
    if (!nav) return;

    const isAdmin = d.user.role === "admin";
    const hasSubscription = isAdmin || !!d.user.hasActiveSubscription;

    const links = [
      ["Dashboard", "/dashboard.html"],
      ...(hasSubscription ? [["Download", "/download.html"]] : []),
      ["Store", "/store.html"],
      ...(isAdmin ? [["Admin", "/admin.html"]] : [])
    ];

    nav.innerHTML = links.slice(0, 5)
      .map(([label, href]) => `<a href="${href}">${label}</a>`).join("");

    if (actions) {
      actions.innerHTML = `
        <div class="account-actions">
          <div class="coin-balance" title="Coin balance">
            <span class="coin-dot"></span><span id="nav-coins">${Number(d.user.coins || 0)}</span> coins
          </div>
          <a class="nav-settings" href="/settings.html">Settings</a>
          <form action="/logout" method="POST"><button class="nav-logout" type="submit">Log out</button></form>
        </div>`;
    }

    const mobileItems = [
      ...links,
      ["Settings", "/settings.html"],
      ["Logout", null]
    ];

    const mobile = document.createElement("div");
    mobile.className = "mobile-nav";
    mobile.innerHTML = `
      <button class="mobile-nav-toggle" type="button" aria-label="Open navigation" aria-expanded="false">
        <span></span><span></span><span></span>
      </button>
      <div class="mobile-nav-panel" aria-hidden="true">
        ${mobileItems.map(([label, href]) =>
          href ? `<a href="${href}">${label}</a>` :
          `<form action="/logout" method="POST"><button type="submit">${label}</button></form>`
        ).join("")}
      </div>`;
    document.querySelector(".topbar-inner")?.appendChild(mobile);

    const toggle = mobile.querySelector(".mobile-nav-toggle");
    const panel = mobile.querySelector(".mobile-nav-panel");
    toggle.addEventListener("click", () => {
      const open = mobile.classList.toggle("open");
      toggle.setAttribute("aria-expanded", String(open));
      panel.setAttribute("aria-hidden", String(!open));
    });

    document.addEventListener("click", e => {
      if (!mobile.contains(e.target)) {
        mobile.classList.remove("open");
        toggle.setAttribute("aria-expanded", "false");
        panel.setAttribute("aria-hidden", "true");
      }
    });
  } catch {}
})();
