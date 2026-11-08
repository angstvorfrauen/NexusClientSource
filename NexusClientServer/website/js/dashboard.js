"use strict";

function formatSubscription(subscription) {
  if (!subscription) return "No active subscription";
  if (subscription.plan === "lifetime") return "Lifetime";

  const expires = new Date(subscription.expiresAt);
  if (Number.isNaN(expires.getTime())) return "No active subscription";

  const now = Date.now();
  if (expires.getTime() <= now) return "Expired";

  return `${subscription.planLabel || subscription.plan || "Active"} · ${expires.toLocaleDateString(undefined, {
    year: "numeric",
    month: "short",
    day: "numeric"
  })}`;
}

async function loadDashboard() {
  try {
    const response = await fetch("/api/me", {
      credentials: "same-origin",
      cache: "no-store"
    });

    if (!response.ok) {
      window.location.replace("/login.html");
      return;
    }

    const data = await response.json();
    if (!data.authenticated || !data.user) {
      window.location.replace("/login.html");
      return;
    }

    const user = data.user;
    const isAdmin = user.role === "admin";

    const username = document.getElementById("username");
    if (username) username.textContent = user.username || "...";

    const accountName = document.getElementById("account-name");
    if (accountName) accountName.textContent = user.username || "...";

    const coins = document.getElementById("coins");
    if (coins) coins.textContent = user.coins ?? 0;

    const license = document.getElementById("license");
    if (license) license.textContent = isAdmin ? "OWNER" : (user.license || "No license");

    const subscription = document.getElementById("subscription-time");
    if (subscription) {
      subscription.textContent = isAdmin ? "Administrator" : formatSubscription(user.subscription);
    }

    const avatar = document.getElementById("profile-avatar");
    if (avatar) {
      const initial = (user.username || "?").trim().charAt(0).toUpperCase() || "?";
      if (user.avatar) {
        avatar.innerHTML = "";
        const img = document.createElement("img");
        img.src = user.avatar;
        img.alt = user.username + " profile picture";
        img.onerror = () => {
          avatar.innerHTML = `<span class="avatar-initial">${initial}</span>`;
        };
        avatar.appendChild(img);
      } else {
        avatar.innerHTML = `<span class="avatar-initial">${initial}</span>`;
      }
    }
  } catch (error) {
    console.error("Dashboard load failed:", error);
    window.location.replace("/login.html");
  }
}

loadDashboard();
