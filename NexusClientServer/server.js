"use strict";

const http = require("http");
const crypto = require("crypto");
const path = require("path");
const fs = require("fs");
const fsp = fs.promises;
const nodemailer = require("nodemailer");

const PORT = 6767;
const HOST = "0.0.0.0";
const PUBLIC_IP_API = "https://api.ipify.org?format=json";
const PUBLIC_IP_REFRESH = 5 * 60 * 1000;
let cachedPublicIp = null;
let cachedPublicIpAt = 0;
const PUBLIC_DIR = path.join(__dirname, "website");
const DOWNLOAD_DIR = path.join(__dirname, "downloads");
const DATA_DIR = path.join(__dirname, "data");
const USERS_FILE = path.join(DATA_DIR, "users.json");
const LICENSES_FILE = path.join(DATA_DIR, "licenses.json");
const SESSIONS_FILE = path.join(DATA_DIR, "sessions.json");

const ADMIN_USER = process.env.ADMIN_USER || "YOUR_USERNAME";
const ADMIN_PASSWORD = process.env.ADMIN_PASSWORD || "YOUR_PASSWORD";

const MAX_BODY = 1024 * 1024;
const SESSION_TTL = 7 * 24 * 60 * 60 * 1000;
const REQUEST_WINDOW = 60_000;
const REQUEST_LIMIT = 600;
const RESET_TOKEN_TTL = 15 * 60 * 1000;
const TWO_FA_CODE_TTL = 10 * 60 * 1000;
const TRUSTED_DEVICE_TTL = 365 * 24 * 60 * 60 * 1000;
const MAIL_USER = process.env.MAIL_USER || "YOUR_EMAIL@gmail.com";
const MAIL_PASS = process.env.MAIL_PASS || "YOUR_16_TOKEN";
const transporter = nodemailer.createTransport({
  service: "gmail",
  auth: { user: MAIL_USER, pass: MAIL_PASS }
});
const pendingTwoFactor = new Map();
const TELEGRAM_URL = "https://t.me/@Einzelhandelskaufmann";

const PLANS = {
  "7d": { name: "7 Days", coins: 5, days: 7 },
  "30d": { name: "30 Days", coins: 15, days: 30 },
  "3m": { name: "3 Months", coins: 35, days: 90 },
  "lifetime": { name: "Lifetime", coins: 100, days: 36500 },
};

const sessions = new Map();
const requestBuckets = new Map();

function persistSessions() {
  try {
    const serializable = {};
    for (const [token, session] of sessions.entries()) {
      if (!session || !token || Date.now() > Number(session.expiresAt || 0)) continue;
      serializable[token] = {
        username: String(session.username || ""),
        role: String(session.role || "user"),
        createdAt: Number(session.createdAt || Date.now()),
        expiresAt: Number(session.expiresAt || 0)
      };
    }
    const temp = `${SESSIONS_FILE}.tmp`;
    fs.writeFileSync(temp, JSON.stringify(serializable, null, 2), "utf8");
    fs.renameSync(temp, SESSIONS_FILE);
  } catch (error) {
    console.error("Could not persist sessions:", error.message);
  }
}

function loadSessions() {
  try {
    if (!fs.existsSync(SESSIONS_FILE)) return;
    const raw = fs.readFileSync(SESSIONS_FILE, "utf8");
    const stored = JSON.parse(raw || "{}");
    const now = Date.now();
    for (const [token, session] of Object.entries(stored)) {
      if (!token || !session || !session.username) continue;
      if (now >= Number(session.expiresAt || 0)) continue;
      sessions.set(token, {
        username: String(session.username),
        role: session.role === "admin" ? "admin" : "user",
        createdAt: Number(session.createdAt || now),
        expiresAt: Number(session.expiresAt)
      });
    }
    persistSessions();
  } catch (error) {
    console.error("Could not load sessions:", error.message);
  }
}

function deleteSession(token) {
  const deleted = deleteSession(token);
  if (deleted) persistSessions();
  return deleted;
}


const MIMETYPE = {
  ".html": "text/html; charset=utf-8",
  ".js": "application/javascript; charset=utf-8",
  ".css": "text/css; charset=utf-8",
  ".json": "application/json; charset=utf-8",
  ".png": "image/png",
  ".jpg": "image/jpeg",
  ".jpeg": "image/jpeg",
  ".webp": "image/webp",
  ".ico": "image/x-icon",
  ".ttf": "font/ttf",
  ".svg": "image/svg+xml",
};

function send(res, status, body, type = "text/plain; charset=utf-8", extra = {}) {
  res.writeHead(status, { "Content-Type": type, "X-Content-Type-Options": "nosniff", ...extra });
  res.end(body);
}
function redirect(res, location, extra = {}) {
  res.writeHead(303, { Location: location, "Cache-Control": "no-store", ...extra });
  res.end();
}
function json(res, status, obj) {
  send(res, status, JSON.stringify(obj), "application/json; charset=utf-8", { "Cache-Control": "no-store" });
}
function parseCookies(req) {
  const result = {};
  for (const part of String(req.headers.cookie || "").split(";")) {
    const i = part.indexOf("=");
    if (i !== -1) result[part.slice(0, i).trim()] = decodeURIComponent(part.slice(i + 1).trim());
  }
  return result;
}
function sessionFor(req) {
  const cookies = parseCookies(req);
  let token = cookies.nexus_session || "";

  if (!token) {
    const authorization = String(req.headers.authorization || "");
    if (/^Bearer\s+/i.test(authorization)) {
      token = authorization.replace(/^Bearer\s+/i, "").trim();
    }
  }

  if (!token) return null;
  const session = sessions.get(token);
  if (!session) return null;
  if (Date.now() > session.expiresAt) { deleteSession(token); return null; }
  return { token, ...session };
}
function createSession(username, role = "user") {
  const token = crypto.randomBytes(32).toString("hex");
  const now = Date.now();
  sessions.set(token, { username, role, createdAt: now, expiresAt: now + SESSION_TTL });
  persistSessions();
  return token;
}
function cookie(token, maxAge = SESSION_TTL / 1000) {
  return `nexus_session=${encodeURIComponent(token)}; Path=/; HttpOnly; SameSite=Lax; Max-Age=${maxAge}`;
}
function trustedDeviceCookie(token, maxAge = TRUSTED_DEVICE_TTL / 1000) {
  return `nexus_trusted=${encodeURIComponent(token)}; Path=/; HttpOnly; SameSite=Lax; Max-Age=${maxAge}`;
}
function clearTrustedDeviceCookie() {
  return "nexus_trusted=; Path=/; HttpOnly; SameSite=Lax; Max-Age=0";
}
function createTrustedDeviceToken() {
  return crypto.randomBytes(32).toString("hex");
}
function isTrustedDevice(req, user) {
  const token = parseCookies(req).nexus_trusted;
  if (!token || !user.trustedDeviceHash) return false;
  const hash = hashToken(token);
  return crypto.timingSafeEqual(Buffer.from(hash), Buffer.from(String(user.trustedDeviceHash)));
}
function hashPassword(password, salt = crypto.randomBytes(16).toString("hex")) {
  return new Promise((resolve, reject) => crypto.scrypt(password, salt, 64, (err, key) => err ? reject(err) : resolve(`${salt}:${key.toString("hex")}`)));
}
function verifyPassword(password, stored) {
  return new Promise((resolve, reject) => {
    const [salt, key] = String(stored).split(":");
    if (!salt || !key) return resolve(false);
    crypto.scrypt(password, salt, 64, (err, derived) => {
      if (err) return reject(err);
      const expected = Buffer.from(key, "hex");
      resolve(expected.length === derived.length && crypto.timingSafeEqual(expected, derived));
    });
  });
}
async function loadUsers() {
  try { const users = JSON.parse(await fsp.readFile(USERS_FILE, "utf8")); return users && typeof users === "object" ? users : {}; }
  catch (e) { if (e.code === "ENOENT") return {}; throw e; }
}
let usersSaveChain = Promise.resolve();
async function saveUsers(users) {
  const run = usersSaveChain.then(async () => {
    await fsp.mkdir(DATA_DIR, { recursive: true });
    const tmp = `${USERS_FILE}.${process.pid}.${Date.now()}.${crypto.randomBytes(4).toString("hex")}.tmp`;
    await fsp.writeFile(tmp, JSON.stringify(users, null, 2), "utf8");
    await fsp.rename(tmp, USERS_FILE);
  });
  usersSaveChain = run.catch(() => {});
  return run;
}
async function loadLicenses() {
  try { const licenses = JSON.parse(await fsp.readFile(LICENSES_FILE, "utf8")); return Array.isArray(licenses) ? licenses : []; }
  catch (e) { if (e.code === "ENOENT") return []; throw e; }
}
async function saveLicenses(licenses) {
  await fsp.mkdir(DATA_DIR, { recursive: true });
  const tmp = `${LICENSES_FILE}.tmp`;
  await fsp.writeFile(tmp, JSON.stringify(licenses, null, 2), "utf8");
  await fsp.rename(tmp, LICENSES_FILE);
}
function generateLicense() {
  return `NX-${crypto.randomBytes(3).toString("hex").toUpperCase()}-${crypto.randomBytes(3).toString("hex").toUpperCase()}-${crypto.randomBytes(3).toString("hex").toUpperCase()}-${crypto.randomBytes(3).toString("hex").toUpperCase()}`;
}
function validUsername(v) { return /^[a-zA-Z0-9_]{3,24}$/.test(v); }
function validEmail(v) { return v === "" || /^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(v); }
function validLicense(v) { return /^[A-Za-z0-9-]{4,80}$/.test(v); }
function readBody(req) {
  return new Promise((resolve, reject) => {
    let body = "", size = 0;
    req.on("data", chunk => {
      size += chunk.length;
      if (size > MAX_BODY) { reject(new Error("Request body too large")); req.destroy(); return; }
      body += chunk.toString("utf8");
    });
    req.on("end", () => resolve(body));
    req.on("error", reject);
  });
}
async function formData(req) { return new URLSearchParams(await readBody(req)); }
function rateLimit(req, res) {
  const ip = req.socket.remoteAddress || "unknown";
  const now = Date.now();
  let b = requestBuckets.get(ip);
  if (!b || now - b.startedAt >= REQUEST_WINDOW) { b = { startedAt: now, count: 0 }; requestBuckets.set(ip, b); }
  if (++b.count > REQUEST_LIMIT) { send(res, 429, "Too many requests", "text/plain; charset=utf-8", { "Retry-After": "60" }); return false; }
  return true;
}
async function requireUser(req, res) {
  const s = sessionFor(req);
  if (!s) { json(res, 401, { error: "Authentication required." }); return null; }
  if (s.role === "admin") return s;
  return s;
}
async function requireAdmin(req, res) {
  const s = sessionFor(req);
  if (!s || s.role !== "admin") { json(res, 401, { error: "Admin authentication required." }); return null; }
  return s;
}


function getClientIp(req) {
  const forwarded = String(req.headers["x-forwarded-for"] || "").split(",")[0].trim();
  const socketIp = String(req.socket.remoteAddress || "unknown");
  return forwarded || socketIp.replace(/^::ffff:/, "") || "unknown";
}

async function getPublicIp() {
  if (cachedPublicIp && Date.now() - cachedPublicIpAt < PUBLIC_IP_REFRESH) return cachedPublicIp;

  try {
    const response = await fetch(PUBLIC_IP_API, { headers: { "Accept": "application/json" } });
    if (!response.ok) throw new Error(`IP API returned ${response.status}`);
    const data = await response.json();
    if (!data || typeof data.ip !== "string" || !data.ip.trim()) throw new Error("Invalid IP API response");
    cachedPublicIp = data.ip.trim();
    cachedPublicIpAt = Date.now();
    return cachedPublicIp;
  } catch (error) {
    console.warn("Could not resolve public IP:", error.message);
    return null;
  }
}

async function getStoredIp(req) {
  return (await getPublicIp()) || getClientIp(req);
}

function sanitizeHwid(value) {
  const v = String(value || "").trim();
  if (!v) return null;
  return v.slice(0, 256);
}

function publicUser(user) {
  return {
    username: user.username,
    email: user.email || "",
    role: user.role || "user",
    coins: Number(user.coins || 0),
    subscription: user.subscription || null,
    hasActiveSubscription: !!(user.subscription && new Date(user.subscription.expiresAt).getTime() > Date.now()),
    createdAt: user.createdAt || null,
    createdIp: user.createdIp || user.lastIp || "Unknown",
    lastIp: user.lastIp || user.createdIp || "Unknown",
    hwid: user.hwid || null,
    banned: user.banned === true,
    userAgent: user.userAgent || "",
    avatar: user.avatar || null,
  };
}


function createNumericCode() {
  return String(crypto.randomInt(100000, 1000000));
}

function hashToken(value) {
  return crypto.createHash("sha256").update(String(value)).digest("hex");
}

async function sendEmail(to, subject, text) {
  await transporter.sendMail({
    from: MAIL_USER,
    to,
    subject,
    text
  });
}

async function sendTwoFactorCode(email, code) {
  await sendEmail(
    email,
    "NexusClient verification code",
    `Your NexusClient verification code is: ${code}\n\nThis code expires in 10 minutes.\nIf you did not request this code, you can ignore this email.`
  );
}

function getPublicOrigin(req) {
  const configured = String(process.env.PUBLIC_URL || "").trim().replace(/\/$/, "");
  if (configured) return configured;

  const forwardedProto = String(req.headers["x-forwarded-proto"] || "").split(",")[0].trim();
  const protocol = forwardedProto || (req.socket.encrypted ? "https" : "http");
  const forwardedHost = String(req.headers["x-forwarded-host"] || "").split(",")[0].trim();
  const host = forwardedHost || String(req.headers.host || `localhost:${PORT}`);

  return `${protocol}://${host}`;
}

async function sendResetEmail(req, email, username, token) {
  const host = getPublicOrigin(req);
  const link = `${host}/reset-password.html?token=${encodeURIComponent(token)}`;
  await sendEmail(
    email,
    "NexusClient password reset",
    `Hello ${username},\n\nA password reset was requested for your NexusClient account.\n\nReset your password here:\n${link}\n\nThis link expires in 15 minutes. If you did not request this, you can ignore this email.`
  );
}

function cleanupSecurityMaps() {
  const now = Date.now();
  for (const [key, value] of pendingTwoFactor) {
    if (value.expiresAt <= now) pendingTwoFactor.delete(key);
  }
}

async function handleForgotPassword(req, res) {
  const data = await formData(req);
  const email = String(data.get("email") || "").trim().toLowerCase();
  if (!validEmail(email) || !email) return send(res, 400, "Enter a valid e-mail address.");

  const users = await loadUsers();
  const user = Object.values(users).find(x => String(x.email || "").toLowerCase() === email);

  if (user) {
    const rawToken = crypto.randomBytes(32).toString("hex");
    user.resetTokenHash = hashToken(rawToken);
    user.resetTokenExpiresAt = Date.now() + RESET_TOKEN_TTL;
    await saveUsers(users);
    try {
      await sendResetEmail(req, email, user.username, rawToken);
    } catch (error) {
      console.error("Reset e-mail failed:", error.message);
      user.resetTokenHash = null;
      user.resetTokenExpiresAt = null;
      await saveUsers(users);
      return send(res, 500, "Could not send the reset e-mail.");
    }
  }

  return json(res, 200, {
    success: true,
    message: "If an account with that e-mail exists, a reset link has been sent."
  });
}

async function handleResetPassword(req, res) {
  const data = await formData(req);
  const token = String(data.get("token") || "").trim();
  const password = String(data.get("password") || "");

  if (!/^[a-f0-9]{64}$/i.test(token)) return send(res, 400, "Invalid or expired reset link.");
  if (password.length < 6 || password.length > 256) return send(res, 400, "Password must contain 6-256 characters.");

  const users = await loadUsers();
  const tokenHash = hashToken(token);
  const user = Object.values(users).find(x =>
    x.resetTokenHash === tokenHash &&
    Number(x.resetTokenExpiresAt || 0) > Date.now()
  );

  if (!user) return send(res, 400, "Invalid or expired reset link.");

  user.passwordHash = await hashPassword(password);
  user.resetTokenHash = null;
  user.resetTokenExpiresAt = null;
  user.twoFactorPending = null;
  await saveUsers(users);

  for (const [key, session] of sessions) {
    if (session.username.toLowerCase() === user.username.toLowerCase()) deleteSession(key);
  }

  return json(res, 200, { success: true, message: "Password changed successfully." });
}

async function handleTwoFactorSetup(req, res) {
  const session = await requireUser(req, res);
  if (!session) return;
  if (session.role === "admin") return json(res, 400, { error: "2FA cannot be changed for the built-in admin account." });

  const users = await loadUsers();
  const user = users[session.username.toLowerCase()];
  if (!user) return json(res, 404, { error: "User not found." });
  if (!user.email) return json(res, 400, { error: "Add an e-mail address to your profile first." });
  if (user.twoFactorEnabled) return json(res, 400, { error: "2FA is already enabled." });

  cleanupSecurityMaps();
  const code = createNumericCode();
  const pendingId = crypto.randomBytes(24).toString("hex");
  pendingTwoFactor.set(pendingId, {
    username: user.username,
    code,
    expiresAt: Date.now() + TWO_FA_CODE_TTL,
    attempts: 0
  });

  try {
    await sendTwoFactorCode(user.email, code);
  } catch (error) {
    pendingTwoFactor.delete(pendingId);
    console.error("2FA setup e-mail failed:", error.message);
    return json(res, 500, { error: "Could not send the verification e-mail." });
  }

  return json(res, 200, { success: true, pendingId, message: "A verification code was sent to your e-mail." });
}

async function handleTwoFactorVerify(req, res) {
  const session = await requireUser(req, res);
  if (!session) return;
  if (session.role === "admin") return json(res, 400, { error: "2FA cannot be changed for the built-in admin account." });

  const data = await jsonBody(req);
  const pendingId = String(data.pendingId || "");
  const code = String(data.code || "").trim();
  const pending = pendingTwoFactor.get(pendingId);

  if (!pending || pending.expiresAt <= Date.now() || pending.username.toLowerCase() !== session.username.toLowerCase()) {
    pendingTwoFactor.delete(pendingId);
    return json(res, 400, { error: "Invalid or expired verification code." });
  }

  pending.attempts++;
  if (pending.attempts > 5) {
    pendingTwoFactor.delete(pendingId);
    return json(res, 429, { error: "Too many incorrect attempts. Request a new code." });
  }

  if (!/^\d{6}$/.test(code) || !crypto.timingSafeEqual(Buffer.from(code), Buffer.from(pending.code))) {
    return json(res, 400, { error: "Incorrect verification code." });
  }

  const users = await loadUsers();
  const user = users[session.username.toLowerCase()];
  if (!user) return json(res, 404, { error: "User not found." });

  user.twoFactorEnabled = true;
  await saveUsers(users);
  pendingTwoFactor.delete(pendingId);
  return json(res, 200, { success: true, enabled: true });
}

async function handleTwoFactorDisable(req, res) {
  const session = await requireUser(req, res);
  if (!session) return;
  if (session.role === "admin") return json(res, 400, { error: "2FA cannot be changed for the built-in admin account." });

  const data = await jsonBody(req);
  const code = String(data.code || "").trim();
  const users = await loadUsers();
  const user = users[session.username.toLowerCase()];
  if (!user) return json(res, 404, { error: "User not found." });
  if (!user.twoFactorEnabled) return json(res, 400, { error: "2FA is already disabled." });

  const expected = String(user.twoFactorRecoveryCode || "");
  if (!/^\d{6}$/.test(code) || !expected || !crypto.timingSafeEqual(Buffer.from(code), Buffer.from(expected))) {
    return json(res, 400, { error: "Invalid verification code." });
  }

  user.twoFactorEnabled = false;
  user.twoFactorRecoveryCode = null;
  await saveUsers(users);
  return json(res, 200, { success: true, enabled: false });
}

async function handleTwoFactorLogin(req, res) {
  cleanupSecurityMaps();
  const data = await formData(req);
  const pendingId = String(data.get("pendingId") || "");
  const code = String(data.get("code") || "").trim();
  const pending = pendingTwoFactor.get(pendingId);

  if (!pending || pending.expiresAt <= Date.now()) {
    pendingTwoFactor.delete(pendingId);
    return send(res, 400, "Invalid or expired 2FA code.");
  }

  pending.attempts++;
  if (pending.attempts > 5) {
    pendingTwoFactor.delete(pendingId);
    return send(res, 429, "Too many incorrect attempts. Please log in again.");
  }

  if (!/^\d{6}$/.test(code) || !crypto.timingSafeEqual(Buffer.from(code), Buffer.from(pending.code))) {
    return send(res, 401, "Incorrect 2FA code.");
  }

  const users = await loadUsers();
  const user = users[pending.username.toLowerCase()];
  pendingTwoFactor.delete(pendingId);
  if (!user || user.banned === true) return send(res, 403, "Account is unavailable.");

  const trustedToken = createTrustedDeviceToken();
  user.trustedDeviceHash = hashToken(trustedToken);
  user.lastIp = await getStoredIp(req);
  user.userAgent = String(req.headers["user-agent"] || "").slice(0, 500);
  await saveUsers(users);

  const token = createSession(user.username, "user");
  res.writeHead(303, {
    Location: "/dashboard.html",
    "Set-Cookie": [cookie(token), trustedDeviceCookie(trustedToken)],
    "Cache-Control": "no-store"
  });
  res.end();
}

async function handleRegister(req, res) {
  const data = await formData(req);
  const username = (data.get("username") || "").trim();
  const email = (data.get("email") || "").trim().toLowerCase();
  const password = data.get("password") || "";
  const license = (data.get("license") || "").trim();
  const terms = data.get("terms") === "on";
  if (!validUsername(username)) return send(res, 400, "Invalid username.");
  if (password.length < 6 || password.length > 256) return send(res, 400, "Password must contain 6-256 characters.");
  if (!validEmail(email)) return send(res, 400, "Invalid email address.");
  if (!validLicense(license)) return send(res, 400, "Invalid license key.");
  if (!terms) return send(res, 400, "You must accept the Terms of Service.");

  const users = await loadUsers();
  const licenses = await loadLicenses();
  const licenseEntry = licenses.find(x => x.key.toLowerCase() === license.toLowerCase());
  if (!licenseEntry || licenseEntry.used) return send(res, 400, "Invalid or already used license key.");
  const key = username.toLowerCase();
  if (users[key]) return send(res, 409, "Username is already registered.");
  for (const user of Object.values(users)) {
    if (email && user.email === email) return send(res, 409, "E-mail is already registered.");
    if (user.license && user.license.toLowerCase() === license.toLowerCase()) return send(res, 409, "License key is already registered.");
  }

  users[key] = { username, email, license, passwordHash: await hashPassword(password), role: "user", coins: 0, subscription: null, createdAt: new Date().toISOString(), createdIp: await getStoredIp(req), lastIp: await getStoredIp(req), userAgent: String(req.headers["user-agent"] || "").slice(0, 500), avatar: null };
  licenseEntry.used = true;
  licenseEntry.usedBy = username;
  licenseEntry.usedAt = new Date().toISOString();
  await saveUsers(users); await saveLicenses(licenses);
  const token = createSession(username);
  res.writeHead(303, { Location: "/dashboard.html", "Set-Cookie": cookie(token), "Cache-Control": "no-store" }); res.end();
}


async function handleLoaderLogin(req, res) {
  const data = await jsonBody(req);
  const username = String(data.username || "").trim();
  const password = String(data.password || "");

  if (!username || !password) return json(res, 400, { error: "Username and password are required." });

  if (username.toLowerCase() === ADMIN_USER.toLowerCase() && password === ADMIN_PASSWORD) {
    const token = createSession(ADMIN_USER, "admin");
    return json(res, 200, {
      success: true,
      token,
      username: ADMIN_USER,
      avatar: null,
      expiresAt: ""
    });
  }

  const users = await loadUsers();
  const user = users[username.toLowerCase()];
  if (!user || !(await verifyPassword(password, user.passwordHash))) {
    return json(res, 401, { error: "Invalid username or password." });
  }
  if (user.banned === true) return json(res, 403, { error: "This account has been banned." });

  if (user.twoFactorEnabled && !isTrustedDevice(req, user)) {
    return json(res, 403, { error: "Two-factor authentication is required. Log in once through the website on this device first." });
  }

  user.lastIp = await getStoredIp(req);
  user.userAgent = String(req.headers["user-agent"] || "").slice(0, 500);
  await saveUsers(users);

  const token = createSession(user.username, "user");
  const activeSubscription = user.subscription &&
    new Date(user.subscription.expiresAt).getTime() > Date.now();

  return json(res, 200, {
    success: true,
    token,
    username: user.username,
    avatar: user.avatar || null,
    expiresAt: activeSubscription ? user.subscription.expiresAt : ""
  });
}

async function handleLoaderSession(req, res) {
  const session = sessionFor(req);
  if (!session) return json(res, 401, { authenticated: false });

  if (session.role === "admin") {
    return json(res, 200, {
      authenticated: true,
      user: {
        username: ADMIN_USER,
        role: "admin",
        avatar: null,
        expiresAt: ""
      }
    });
  }

  const users = await loadUsers();
  const user = users[session.username.toLowerCase()];
  if (!user || user.banned === true) {
    deleteSession(session.token);
    return json(res, 401, { authenticated: false });
  }

  const activeSubscription = user.subscription &&
    new Date(user.subscription.expiresAt).getTime() > Date.now();

  return json(res, 200, {
    authenticated: true,
    user: {
      username: user.username,
      role: user.role || "user",
      avatar: user.avatar || null,
      expiresAt: activeSubscription ? user.subscription.expiresAt : ""
    }
  });
}

async function handleLogin(req, res) {
  const data = await formData(req);
  const username = (data.get("username") || "").trim();
  const password = data.get("password") || "";
  if (username.toLowerCase() === ADMIN_USER.toLowerCase() && password === ADMIN_PASSWORD) {
    const token = createSession(ADMIN_USER, "admin");
    res.writeHead(303, { Location: "/dashboard.html", "Set-Cookie": cookie(token), "Cache-Control": "no-store" }); return res.end();
  }
  const users = await loadUsers();
  const user = users[username.toLowerCase()];
  if (!user || !(await verifyPassword(password, user.passwordHash))) return send(res, 401, "Invalid username or password.");
  if (user.banned === true) return send(res, 403, "This account has been banned.");

  if (user.twoFactorEnabled && !isTrustedDevice(req, user)) {
    if (!user.email) return send(res, 400, "2FA is enabled but no e-mail address is configured.");
    cleanupSecurityMaps();
    const code = createNumericCode();
    const pendingId = crypto.randomBytes(24).toString("hex");
    pendingTwoFactor.set(pendingId, {
      username: user.username,
      code,
      expiresAt: Date.now() + TWO_FA_CODE_TTL,
      attempts: 0
    });
    try {
      await sendTwoFactorCode(user.email, code);
    } catch (error) {
      pendingTwoFactor.delete(pendingId);
      console.error("2FA login e-mail failed:", error.message);
      return send(res, 500, "Could not send the 2FA e-mail.");
    }
    return json(res, 200, { requiresTwoFactor: true, pendingId, reason: "new_device" });
  }

  user.lastIp = await getStoredIp(req);
  user.userAgent = String(req.headers["user-agent"] || "").slice(0, 500);
  await saveUsers(users);
  const token = createSession(user.username, "user");
  res.writeHead(303, { Location: "/dashboard.html", "Set-Cookie": cookie(token), "Cache-Control": "no-store" }); res.end();
}

async function handleMe(req, res) {
  const session = sessionFor(req);
  if (!session) return json(res, 401, { authenticated: false });
  if (session.role === "admin") return json(res, 200, { authenticated: true, user: { username: ADMIN_USER, role: "admin", coins: 0, subscription: null, hasActiveSubscription: true } });
  const users = await loadUsers();
  const user = users[session.username.toLowerCase()];
  if (!user) { sessions.delete(session.token); return json(res, 401, { authenticated: false }); }
  let needsSave = false;
  if (typeof user.coins !== "number") { user.coins = 0; needsSave = true; }
  if (!Object.prototype.hasOwnProperty.call(user, "subscription")) { user.subscription = null; needsSave = true; }
  if (needsSave) await saveUsers(users);
  const hasActiveSubscription = !!(user.subscription && new Date(user.subscription.expiresAt).getTime() > Date.now());
  return json(res, 200, { authenticated: true, user: { username: user.username, email: user.email, license: user.license, coins: user.coins, subscription: user.subscription, hasActiveSubscription, createdAt: user.createdAt, avatar: user.avatar || null } });
}


async function jsonBody(req) {
  const raw = await readBody(req);
  try { return JSON.parse(raw || "{}"); }
  catch { throw new Error("Invalid JSON body"); }
}

function validAvatar(value) {
  if (value == null || value === "") return true;
  return typeof value === "string" &&
    /^data:image\/(?:png|jpeg|webp);base64,[A-Za-z0-9+/=\r\n]+$/.test(value) &&
    value.length <= 300 * 1024 * 1.4;
}

async function handleSettings(req, res) {
  const session = await requireUser(req, res); if (!session) return;
  if (session.role === "admin") {
    return json(res, 200, { user: { username: ADMIN_USER, email: "", role: "admin", coins: 0, lastIp: await getStoredIp(req), userAgent: req.headers["user-agent"] || "", createdAt: null, hwid: null, avatar: null } });
  }
  const users = await loadUsers();
  const oldKey = session.username.toLowerCase();
  const user = users[oldKey];
  if (!user) return json(res, 404, { error: "User not found." });
  if (!user.lastIp) { user.lastIp = await getStoredIp(req); }
  if (!user.createdIp) { user.createdIp = user.lastIp || await getStoredIp(req); }
  if (!user.userAgent) { user.userAgent = String(req.headers["user-agent"] || "").slice(0, 500); }
  return json(res, 200, { user: {
    ...publicUser({ ...user, userAgent: user.userAgent, avatar: user.avatar || null }),
    twoFactorEnabled: user.twoFactorEnabled === true
  } });
}

async function handleSettingsUpdate(req, res) {
  const session = await requireUser(req, res); if (!session) return;
  if (session.role === "admin") return json(res, 400, { error: "The admin profile cannot be edited here." });
  const data = await jsonBody(req);
  const username = String(data.username || "").trim();
  const email = String(data.email || "").trim().toLowerCase();
  const avatar = data.avatar == null ? null : String(data.avatar);
  if (!validUsername(username)) return json(res, 400, { error: "Username must be 3-24 characters and use letters, numbers or underscores." });
  if (!validEmail(email)) return json(res, 400, { error: "Invalid email address." });
  if (!validAvatar(avatar)) return json(res, 400, { error: "Invalid profile picture or file is too large." });

  const users = await loadUsers();
  const oldKey = session.username.toLowerCase();
  const user = users[oldKey];
  if (!user) return json(res, 404, { error: "User not found." });
  const newKey = username.toLowerCase();
  if (newKey !== oldKey && users[newKey]) return json(res, 409, { error: "Username is already registered." });
  for (const [key, other] of Object.entries(users)) {
    if (key !== oldKey && email && other.email === email) return json(res, 409, { error: "E-mail is already registered." });
  }

  user.username = username;
  user.email = email;
  if (avatar) user.avatar = avatar;
  else if (!Object.prototype.hasOwnProperty.call(user, "avatar")) user.avatar = null;

  if (newKey !== oldKey) {
    delete users[oldKey];
    users[newKey] = user;
    for (const sessionEntry of sessions.values()) {
      if (sessionEntry.username.toLowerCase() === oldKey) sessionEntry.username = username;
    }
  }
  await saveUsers(users);
  return json(res, 200, { success: true, user: publicUser({ ...user, userAgent: user.userAgent, avatar: user.avatar || null }) });
}

async function handleAccountDelete(req, res) {
  const session = await requireUser(req, res); if (!session) return;
  if (session.role === "admin") return json(res, 400, { error: "The admin account cannot be deleted here." });
  const users = await loadUsers();
  const key = session.username.toLowerCase();
  const user = users[key];
  if (!user) return json(res, 404, { error: "User not found." });

  const licenses = await loadLicenses();
  const license = licenses.find(x => x.key && user.license && x.key.toLowerCase() === user.license.toLowerCase());
  if (license) {
    license.used = false;
    delete license.usedBy;
    delete license.usedAt;
  }
  delete users[key];
  for (const [token, entry] of sessions.entries()) {
    if (entry.username && entry.username.toLowerCase() === key) deleteSession(token);
  }
  await saveUsers(users);
  await saveLicenses(licenses);
  return json(res, 200, { success: true });
}

async function handlePurchase(req, res) {
  const session = await requireUser(req, res); if (!session) return;
  if (session.role === "admin") return json(res, 400, { error: "Admin accounts do not need subscriptions." });
  const data = await formData(req);
  const plan = PLANS[data.get("plan")];
  if (!plan) return json(res, 400, { error: "Invalid plan." });
  const users = await loadUsers(); const user = users[session.username.toLowerCase()];
  if (!user) return json(res, 404, { error: "User not found." });
  const coins = Number(user.coins || 0);
  if (coins < plan.coins) return json(res, 400, { error: `You dont have enough coins!` });

  const now = Date.now();
  const current = user.subscription && new Date(user.subscription.expiresAt).getTime() > now ? new Date(user.subscription.expiresAt).getTime() : now;
  const expiresAt = new Date(current + plan.days * 86400000).toISOString();
  user.coins = coins - plan.coins;
  user.subscription = { product: "Counter-Strike 2", plan: plan.name, expiresAt, purchasedAt: new Date().toISOString() };
  await saveUsers(users);
  return json(res, 200, { success: true, coins: user.coins, subscription: user.subscription });
}

async function handleDownload(req, res, filename, requireSubscription = true) {
  const session = await requireUser(req, res); if (!session) return;

  if (requireSubscription && session.role !== "admin") {
    const users = await loadUsers();
    const user = users[session.username.toLowerCase()];
    const active = user && user.subscription && new Date(user.subscription.expiresAt).getTime() > Date.now();
    if (!active) return json(res, 403, { error: "An active subscription is required to download the client." });
  }

  const source = path.join(DOWNLOAD_DIR, filename);
  if (!fs.existsSync(source)) {
    return json(res, 404, { error: `The file ${filename} has not been uploaded to the server yet.` });
  }

  const randomName = crypto.randomBytes(12).toString("base64url").replace(/[^A-Za-z0-9]/g, "").slice(0, 16).padEnd(16, "0") + ".exe";
  res.writeHead(200, {
    "Content-Type": "application/octet-stream",
    "Content-Disposition": `attachment; filename="${randomName}"`,
    "Cache-Control": "no-store",
    "X-Content-Type-Options": "nosniff"
  });
  fs.createReadStream(source).pipe(res);
}


async function handleAdminUsers(req, res) {
  if (!await requireAdmin(req, res)) return;
  const users = await loadUsers();
  const list = Object.values(users)
    .filter(u => u && typeof u === "object")
    .map(publicUser)
    .sort((a, b) => String(a.username).localeCompare(String(b.username)));
  return json(res, 200, { users: list });
}

async function handleAdminSetSubscription(req, res) {
  if (!await requireAdmin(req, res)) return;
  const data = await formData(req);
  const username = (data.get("username") || "").trim().toLowerCase();
  const planKey = String(data.get("plan") || "").trim();
  if (!username) return json(res, 400, { error: "Username is required." });
  if (username === ADMIN_USER.toLowerCase()) return json(res, 400, { error: "The admin account does not use subscriptions." });
  const plan = PLANS[planKey];
  if (!plan) return json(res, 400, { error: "Invalid subscription plan." });

  const users = await loadUsers();
  const user = users[username];
  if (!user) return json(res, 404, { error: "User not found." });

  const now = Date.now();
  user.subscription = {
    product: "Counter-Strike 2",
    plan: plan.name,
    expiresAt: new Date(now + plan.days * 86400000).toISOString(),
    purchasedAt: new Date().toISOString(),
    adminGranted: true
  };
  await saveUsers(users);
  return json(res, 200, { success: true, user: publicUser(user) });
}

async function handleAdminDeleteSubscription(req, res) {
  if (!await requireAdmin(req, res)) return;
  const data = await formData(req);
  const username = (data.get("username") || "").trim().toLowerCase();
  if (!username) return json(res, 400, { error: "Username is required." });
  if (username === ADMIN_USER.toLowerCase()) return json(res, 400, { error: "The admin account does not use subscriptions." });

  const users = await loadUsers();
  const user = users[username];
  if (!user) return json(res, 404, { error: "User not found." });
  user.subscription = null;
  await saveUsers(users);
  return json(res, 200, { success: true, user: publicUser(user) });
}

async function handleAdminSetCoins(req, res) {
  if (!await requireAdmin(req, res)) return;
  const data = await formData(req);
  const username = (data.get("username") || "").trim().toLowerCase();
  const coins = Number(data.get("coins"));
  if (!username || !Number.isInteger(coins) || coins < 0 || coins > 100000000) {
    return json(res, 400, { error: "Invalid username or coin balance." });
  }
  const users = await loadUsers();
  const user = users[username];
  if (!user) return json(res, 404, { error: "User not found." });
  user.coins = coins;
  await saveUsers(users);
  return json(res, 200, { success: true, user: publicUser(user) });
}

async function handleAdminBan(req, res) {
  if (!await requireAdmin(req, res)) return;
  const data = await formData(req);
  const username = (data.get("username") || "").trim().toLowerCase();
  const banned = String(data.get("banned") || "true") === "true";
  if (!username) return json(res, 400, { error: "Username is required." });
  if (username === ADMIN_USER.toLowerCase()) return json(res, 400, { error: "The admin account cannot be banned." });
  const users = await loadUsers();
  const user = users[username];
  if (!user) return json(res, 404, { error: "User not found." });
  user.banned = banned;
  await saveUsers(users);
  return json(res, 200, { success: true, user: publicUser(user) });
}

async function handleAdminDeleteHwid(req, res) {
  if (!await requireAdmin(req, res)) return;
  const data = await formData(req);
  const username = (data.get("username") || "").trim().toLowerCase();
  if (!username) return json(res, 400, { error: "Username is required." });
  const users = await loadUsers();
  const user = users[username];
  if (!user) return json(res, 404, { error: "User not found." });
  user.hwid = null;
  await saveUsers(users);
  return json(res, 200, { success: true, user: publicUser(user) });
}

async function handleHwid(req, res) {
  const session = await requireUser(req, res); if (!session) return;
  if (session.role === "admin") return json(res, 400, { error: "Admin accounts do not use HWIDs." });
  const data = await formData(req);
  const hwid = sanitizeHwid(data.get("hwid"));
  if (!hwid) return json(res, 400, { error: "A HWID is required." });
  const users = await loadUsers();
  const user = users[session.username.toLowerCase()];
  if (!user) return json(res, 404, { error: "User not found." });
  if (user.banned === true) return json(res, 403, { error: "This account has been banned." });
  user.hwid = hwid;
  user.lastIp = await getStoredIp(req);
  await saveUsers(users);
  return json(res, 200, { success: true });
}

async function handleAdminLicense(req, res) {
  if (!await requireAdmin(req, res)) return;
  const data = await formData(req); let amount = Number(data.get("amount") || 1);
  amount = Math.max(1, Math.min(100, Number.isInteger(amount) ? amount : 1));
  const licenses = await loadLicenses(); const generated = [];
  for (let i = 0; i < amount; i++) {
    let key; do { key = generateLicense(); } while (licenses.some(x => x.key === key));
    licenses.push({ key, used: false, createdAt: new Date().toISOString() }); generated.push(key);
  }
  await saveLicenses(licenses); return json(res, 201, { success: true, licenses: generated });
}
async function handleAdminLicenses(req, res) {
  if (!await requireAdmin(req, res)) return;
  return json(res, 200, { licenses: await loadLicenses() });
}

async function handleAdminCoins(req, res) {
  if (!await requireAdmin(req, res)) return;
  const data = await formData(req);
  const username = (data.get("username") || "").trim().toLowerCase();
  const amount = Number(data.get("amount"));
  if (!username || !Number.isInteger(amount) || amount < 1 || amount > 10000) {
    return json(res, 400, { error: "Invalid username or coin amount." });
  }
  const users = await loadUsers();
  const user = users[username];
  if (!user) return json(res, 404, { error: "User not found." });
  user.coins = Number(user.coins || 0) + amount;
  await saveUsers(users);
  return json(res, 200, { success: true, username: user.username, coins: user.coins });
}
async function handleLogout(req, res) {
  const s = sessionFor(req); if (s) sessions.delete(s.token);
  redirect(res, "/login.html", { "Set-Cookie": cookie("", 0) });
}

async function serveFile(req, res, pathname) {
  let requested = pathname === "/" ? "/index.html" : pathname;
  const session = sessionFor(req);
  if (requested === "/login.html" || requested === "/register.html") {
    if (session) return redirect(res, "/dashboard.html");
  }
  if (["/dashboard.html", "/download.html", "/store.html", "/settings.html"].includes(requested) && !session) return redirect(res, "/login.html");
  if (requested === "/admin.html" && (!session || session.role !== "admin")) return redirect(res, "/login.html");
  if (requested.includes("..") || requested === "/server.js" || requested.startsWith("/data/")) return send(res, 403, "Forbidden");
  const fullPath = path.resolve(PUBLIC_DIR, "." + requested);
  if (!fullPath.startsWith(path.resolve(PUBLIC_DIR) + path.sep)) return send(res, 403, "Forbidden");
  fs.readFile(fullPath, (err, data) => {
    if (err) {
      const indexPath = path.join(PUBLIC_DIR, "index.html");
      return fs.readFile(indexPath, (e, d) => e ? send(res, 500, "Server error") : send(res, 200, d, "text/html; charset=utf-8"));
    }
    send(res, 200, data, MIMETYPE[path.extname(fullPath).toLowerCase()] || "application/octet-stream");
  });
}

const server = http.createServer(async (req, res) => {
  try {
    const url = new URL(req.url, `http://${req.headers.host || "localhost"}`);
    const pathname = decodeURIComponent(url.pathname);
    const isProtectedRequest = pathname.startsWith("/api/") || pathname.startsWith("/admin/") ||
      pathname === "/login" || pathname === "/register" || pathname === "/logout";
    if (isProtectedRequest && !rateLimit(req, res)) return;
    if (req.method === "POST" && pathname === "/register") return await handleRegister(req, res);
    if (req.method === "POST" && pathname === "/login") return await handleLogin(req, res);
    if (req.method === "POST" && pathname === "/api/loader/login") return await handleLoaderLogin(req, res);
    if (req.method === "GET" && pathname === "/api/loader/session") return await handleLoaderSession(req, res);
    if (req.method === "POST" && pathname === "/api/forgot-password") return await handleForgotPassword(req, res);
    if (req.method === "POST" && pathname === "/api/reset-password") return await handleResetPassword(req, res);
    if (req.method === "POST" && pathname === "/api/2fa/setup") return await handleTwoFactorSetup(req, res);
    if (req.method === "POST" && pathname === "/api/2fa/verify") return await handleTwoFactorVerify(req, res);
    if (req.method === "POST" && pathname === "/api/2fa/disable") return await handleTwoFactorDisable(req, res);
    if (req.method === "POST" && pathname === "/api/2fa/login") return await handleTwoFactorLogin(req, res);
    if (req.method === "POST" && pathname === "/logout") return await handleLogout(req, res);
    if (req.method === "GET" && pathname === "/api/me") return await handleMe(req, res);
    if (req.method === "GET" && pathname === "/api/settings") return await handleSettings(req, res);
    if (req.method === "POST" && pathname === "/api/settings") return await handleSettingsUpdate(req, res);
    if (req.method === "POST" && pathname === "/api/account/delete") return await handleAccountDelete(req, res);
    if (req.method === "POST" && pathname === "/api/store/purchase") return await handlePurchase(req, res);
    if (req.method === "GET" && pathname === "/download/loader") return await handleDownload(req, res, "loader.exe", true);
    if (req.method === "GET" && pathname === "/download/client") return await handleDownload(req, res, "app.exe", true);
    if (req.method === "GET" && pathname === "/admin/users") return await handleAdminUsers(req, res);
    if (req.method === "POST" && pathname === "/admin/coins/set") return await handleAdminSetCoins(req, res);
    if (req.method === "POST" && pathname === "/admin/users/subscription/set") return await handleAdminSetSubscription(req, res);
    if (req.method === "POST" && pathname === "/admin/users/subscription/delete") return await handleAdminDeleteSubscription(req, res);
    if (req.method === "POST" && pathname === "/admin/users/ban") return await handleAdminBan(req, res);
    if (req.method === "POST" && pathname === "/admin/users/delete-hwid") return await handleAdminDeleteHwid(req, res);
    if (req.method === "POST" && pathname === "/api/hwid") return await handleHwid(req, res);
    if (req.method === "POST" && pathname === "/admin/licenses") return await handleAdminLicense(req, res);
    if (req.method === "GET" && pathname === "/admin/licenses") return await handleAdminLicenses(req, res);
    if (req.method === "POST" && pathname === "/admin/coins") return await handleAdminCoins(req, res);
    if (req.method !== "GET" && req.method !== "HEAD") return send(res, 405, "Method Not Allowed", "text/plain; charset=utf-8", { Allow: "GET, HEAD, POST" });
    return await serveFile(req, res, pathname);
  } catch (err) { console.error(err); if (!res.headersSent) send(res, 500, "Internal server error"); }
});

async function main() {
  await fsp.mkdir(PUBLIC_DIR, { recursive: true });
  await fsp.mkdir(DOWNLOAD_DIR, { recursive: true });
  await fsp.mkdir(DATA_DIR, { recursive: true });
  loadSessions();
  if (!fs.existsSync(USERS_FILE)) await saveUsers({});
  if (!fs.existsSync(LICENSES_FILE)) await saveLicenses([]);
  getPublicIp().then(ip => {
    const host = ip || "<public-ip>";
    console.log(`NexusClient server running on http://${host}:${PORT}`);
  }).catch(() => {
    console.log(`NexusClient server running on port ${PORT}`);
  });
  setInterval(() => {
    getPublicIp().then(ip => {
      if (ip) console.log(`Public IP: ${ip}:${PORT}`);
    }).catch(() => {});
  }, PUBLIC_IP_REFRESH).unref();

  server.listen(PORT, HOST);
}
main().catch(err => { console.error(err); process.exit(1); });
