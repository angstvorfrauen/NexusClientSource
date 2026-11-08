"use strict";

const form = document.getElementById("reset-form");
const passwordInput = document.getElementById("password");
const confirmInput = document.getElementById("confirm");
const button = document.getElementById("reset-button");
const message = document.getElementById("message");
const token = new URLSearchParams(window.location.search).get("token");

function showMessage(text, error = false) {
  message.textContent = text;
  message.className = `reset-message ${error ? "error" : "success"}`;
}

function updateButton() {
  button.disabled = !passwordInput.value || !confirmInput.value;
}

passwordInput.addEventListener("input", updateButton);
confirmInput.addEventListener("input", updateButton);
updateButton();

form.addEventListener("submit", async (event) => {
  event.preventDefault();

  if (!token || !/^[a-f0-9]{64}$/i.test(token)) {
    showMessage("This reset link is invalid or expired.", true);
    return;
  }

  const password = passwordInput.value;
  const confirm = confirmInput.value;

  if (password.length < 6) {
    showMessage("Password must contain at least 6 characters.", true);
    return;
  }

  if (password !== confirm) {
    showMessage("Passwords do not match.", true);
    return;
  }

  button.disabled = true;

  try {
    const response = await fetch("/api/reset-password", {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded;charset=UTF-8" },
      body: new URLSearchParams({ token, password })
    });

    const text = await response.text();
    if (!response.ok) throw new Error(text || "Could not change your password.");

    showMessage("Password changed successfully. Redirecting to login...");
    form.reset();
    setTimeout(() => { window.location.href = "/login.html"; }, 1400);
  } catch (error) {
    showMessage(error.message || "Could not change your password.", true);
    updateButton();
  }
});
