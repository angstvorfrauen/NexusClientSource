 "use strict";

const form = document.getElementById("register-form");
const button = document.getElementById("register-button");
const username = document.getElementById("username");
const email = document.getElementById("email");
const password = document.getElementById("password");
const license = document.getElementById("license");
const terms = document.getElementById("terms");

function showError(message) {
  let box = document.getElementById("form-message");
  if (!box) {
    box = document.createElement("div");
    box.id = "form-message";
    box.className = "form-message error";
    form.prepend(box);
  }
  box.textContent = message;
  box.hidden = false;
}

function updateButton() {
  const usernameValid = /^[a-zA-Z0-9_]{3,24}$/.test(username.value.trim());
  const passwordValid = password.value.length >= 6 && password.value.length <= 256;
  const licenseValid = /^[A-Za-z0-9-]{4,80}$/.test(license.value.trim());
  const emailValid = email.value.trim() === "" || email.validity.valid;

  button.disabled = !(
    usernameValid &&
    passwordValid &&
    licenseValid &&
    emailValid &&
    terms.checked
  );
}

[username, email, password, license].forEach(el => el.addEventListener("input", updateButton));
terms.addEventListener("change", updateButton);

form.addEventListener("submit", async (event) => {
  event.preventDefault();
  updateButton();
  if (button.disabled) return;

  button.disabled = true;

  try {
    const body = new URLSearchParams({
      username: username.value.trim(),
      email: email.value.trim(),
      password: password.value,
      license: license.value.trim(),
      terms: "on"
    });

    const response = await fetch("/register", {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded;charset=UTF-8" },
      body
    });

    if (response.redirected) {
      window.location.href = response.url;
      return;
    }

    showError(await response.text() || "Account creation failed.");
  } catch {
    showError("Could not connect to the server.");
  } finally {
    updateButton();
  }
});

updateButton();
