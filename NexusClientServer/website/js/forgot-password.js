"use strict";

const form = document.getElementById("forgot-form");
const email = document.getElementById("email");
const message = document.getElementById("message");
const button = document.getElementById("reset-button");

function showMessage(text, error = false) {
  message.textContent = text;
  message.className = `reset-message ${error ? "error" : "success"}`;
  message.hidden = false;
}

form.addEventListener("submit", async (event) => {
  event.preventDefault();

  const value = email.value.trim();

  if (!value) {
    showMessage("Please enter your e-mail address.", true);
    email.focus();
    return;
  }

  button.disabled = true;
  button.querySelector(".button-text").textContent = "Sending...";

  try {
    const response = await fetch("/api/forgot-password", {
      method: "POST",
      headers: {
        "Content-Type": "application/x-www-form-urlencoded;charset=UTF-8"
      },
      body: new URLSearchParams({ email: value })
    });

    const text = await response.text();
    let data;

    try {
      data = JSON.parse(text);
    } catch {
      data = { message: text };
    }

    if (!response.ok) {
      throw new Error(data.message || "Could not send reset link.");
    }

    showMessage(data.message || "Check your e-mail for the reset link.");
    button.querySelector(".button-text").textContent = "Reset link sent";
  } catch (error) {
    showMessage(error.message || "Could not send reset link.", true);
    button.disabled = false;
    button.querySelector(".button-text").textContent = "Send reset link";
  }
});
