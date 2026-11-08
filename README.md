# NexusClient

<p align="center">
  <img src="logo_transparent.png" width="180" alt="NexusClient">
</p>

<h3 align="center">NexusClient – Complete Project Repository</h3>

<p align="center">
  A collection of interconnected C++ applications, a Windows loader, a Node.js backend and a web-based account management platform.
</p>

---

## Table of Contents

- [Overview](#overview)
- [Project Architecture](#project-architecture)
- [1. NexusClient External Source](#1-nexusclient-external-source)
- [2. NexusClient Loader](#2-nexusclient-loader)
- [3. NexusClient Server & Website](#3-nexusclient-server--website)
- [4. Offsets Dumper](#4-offsets-dumper)
- [Communication Between Projects](#communication-between-projects)
- [Technologies](#technologies)
- [Installation & Building](#installation--building)
- [Disclaimer](#disclaimer)

---

## Overview

NexusClient is a multi-component Windows software project consisting of several independent but interconnected applications.

The repository contains the client application, its dedicated launcher, a backend authentication system, a web dashboard and an offsets utility.

Each component has its own responsibilities and source structure.

## Project Architecture

```text
NexusClient/
│
├── NexusClient External Source/
│   ├── NexusClient.sln
│   └── NexusClient/
│       ├── src/
│       │   ├── aimbot/
│       │   ├── config/
│       │   ├── images/
│       │   ├── imgui/
│       │   ├── math/
│       │   ├── menu/
│       │   ├── misc/
│       │   ├── offsets/
│       │   ├── settings/
│       │   ├── toast/
│       │   └── visuals/
│
├── NexusClientLoader/
│   └── nexusclientloader/
│       ├── src/
│       └── nexusclientloader.sln
│
├── NexusClientServer/
│   ├── data/
│   ├── downloads/
│   ├── website/
│   ├── server.js
│   └── package.json
│
├── offsetsdumper/
│   └── cs2-dumper.exe
│
├── logo.png
└── logo_transparent.png
```

---

# 1. NexusClient External Source

**Location:** `NexusClient External Source/`

The External Source is the main C++ application.

It uses Windows APIs, DirectX 11 and Dear ImGui to create a custom graphical interface and provide its different modules.

### Aimbot

**Location:** `src/aimbot/`

The Aimbot module handles targeting-related functionality.

Responsibilities include:

- Target selection
- Field of View configuration
- Aim smoothing
- Bone selection
- Keybind configuration
- Target prioritization
- Aim prediction

The module uses configurable settings that can be changed through the user interface.

### Visuals / ESP

**Location:** `src/visuals/`

The Visuals module handles the rendering of additional player information.

Its components include:

| Component | Responsibility |
|---|---|
| ESP | Main ESP functionality |
| Box | Player bounding boxes |
| Bones | Skeleton visualization |
| Health | Health information |
| Info | Additional player information |
| Lines | Line rendering |
| Name | Player name rendering |

Enemy and teammate settings can be configured separately.

### Miscellaneous

**Location:** `src/misc/`

Contains additional gameplay-related modules.

- **AntiFlash:** Flash effect handling.
- **Bomb Timer:** Bomb timer functionality.
- **Radar:** Radar-related functionality.

### Configuration System

**Location:** `src/config/`

The configuration module manages application settings.

It provides:

- Saving configurations
- Loading configurations
- Applying saved settings
- Managing Aimbot settings
- Managing ESP settings
- Managing miscellaneous settings
- Storing visual preferences

Configurations are handled through the `FullConfig` structure.

### User Interface

**Location:** `src/menu/`

The menu is built using Dear ImGui.

It contains separate tabs for:

- Aimbot
- Visuals
- Miscellaneous
- World
- Configuration
- Settings

Additional UI helpers and styling are implemented separately.

### Math

**Location:** `src/math/`

Provides mathematical utilities used by the application.

Includes:

- Vector operations
- Mathematical calculations
- Color handling
- Coordinate-related functionality

### Offsets

**Location:** `src/offsets/`

Contains CS2-related offsets and global references used by the application.

This includes:

- Client DLL definitions
- Global variables
- Offset definitions

### Other Components

**Location:** `src/`

- `memory.hpp` – Memory-related functionality.
- `console.hpp` – Console utilities.
- `toast/` – Notification system.
- `settings/watermark/` – Watermark functionality.
- `images/` – Application icons and images.

---

# 2. NexusClient Loader

**Location:** `NexusClientLoader/`

The NexusClient Loader is a separate Windows application written in C++.

It acts as the entry point for users and communicates with the NexusClient backend.

### Authentication

**File:** `src/auth.cpp`

The authentication module handles communication with the server.

The login process works as follows:

1. The user enters their username and password.
2. The loader retrieves the machine HWID.
3. The loader sends the authentication request to the backend.
4. The server validates the credentials and account.
5. The loader receives the authentication response.
6. The authenticated user is redirected to the main loader interface.

The loader uses Windows WinHTTP for HTTPS communication.

### Session Management

The loader supports persistent authentication sessions.

Its session system:

- Saves authentication tokens.
- Restores previously saved sessions.
- Validates existing sessions with the backend.
- Clears authentication data when logging out.

Authentication tokens are protected using the Windows Data Protection API (DPAPI).

Session data is stored inside the user's Windows AppData directory.

### Download System

**File:** `src/download.cpp`

The download module communicates with the server to retrieve application files.

Its responsibilities include:

- Requesting protected downloads.
- Downloading files from the backend.
- Handling HTTP responses.
- Checking downloaded Windows executable files.
- Validating PE file signatures and supported architectures.

### Profile System

**File:** `src/profile.cpp`

Handles the user's profile picture.

Features include:

- Processing Base64 image data.
- Converting image data into bitmaps.
- Rendering profile pictures.
- Displaying the username initial when no profile picture exists.

### Main Interface

**File:** `src/main_ui.cpp`

The main loader interface displays the authenticated user's information.

It includes:

- Username
- Profile picture
- Subscription information
- Application version
- Start functionality
- Profile-related controls

### Additional Loader Files

| File | Purpose |
|---|---|
| `main.cpp` | Application entry point |
| `login_ui.cpp` | Login interface |
| `auth.cpp` | Authentication and sessions |
| `download.cpp` | Download functionality |
| `profile.cpp` | Profile picture handling |
| `state.cpp` | Global application state |
| `style.cpp` | UI styling |
| `utils.cpp` | Utility functions |
| `windows.cpp` | Windows-related functionality |
| `version.cpp` | Version information |

---

# 3. NexusClient Server & Website

**Location:** `NexusClientServer/`

The backend connects the website, loader and client ecosystem.

It is built with Node.js and uses a custom HTTP server.

**Main file:** `server.js`

### Backend Architecture

The backend handles:

- User registration
- User authentication
- Session management
- Password hashing
- HWID handling
- Subscription management
- License management
- Coin management
- Protected downloads
- Admin operations
- Password recovery
- Two-factor authentication

### Authentication System

The website and loader use the backend for authentication.

Passwords are hashed using Node.js `crypto.scrypt`.

The backend creates session tokens and validates authenticated requests.

Sessions are persisted in JSON files so they can survive server restarts.

The loader uses dedicated API endpoints for login and session validation.

### User Management

**Location:** `data/users.json`

The user database stores account information.

The backend supports:

- Creating accounts
- Authenticating users
- Updating account settings
- Managing profile information
- Managing account balances
- Account deletion
- HWID-related operations

### License Management

**Location:** `data/licenses.json`

The license system manages generated license keys.

It supports:

- License generation
- License storage
- License administration
- License-related account operations

### Session Storage

**Location:** `data/sessions.json`

Stores persistent authentication session information.

Session entries contain information such as:

- Session token
- Username
- User role
- Creation timestamp
- Expiration timestamp

### Store & Subscription System

The backend contains a subscription and coin-based purchase system.

Configured plans include:

| Plan | Coins | Duration |
|---|---:|---|
| 7 Days | 5 | 7 Days |
| 30 Days | 15 | 30 Days |
| 3 Months | 35 | 90 Days |
| Lifetime | 100 | 36500 Days |

Purchases are processed through the backend.

The server is responsible for validating purchases and updating subscription information.

### Admin System

The admin backend provides administrative functionality.

Available operations include:

- Viewing registered users
- Managing user coins
- Changing subscriptions
- Deleting subscriptions
- Managing user HWIDs
- Banning users
- Managing licenses

Administrative operations are protected by role-based access checks.

### Download System

The backend provides protected download endpoints.

The loader and client downloads are served through dedicated routes.

Download access is handled by the backend rather than exposing the download directory directly.

### Email & Security Features

The backend also includes:

- Email-based password recovery
- Password reset tokens
- Two-factor authentication
- Trusted device handling
- Request rate limiting
- Session expiration

### Website Frontend

**Location:** `website/`

The website provides the graphical interface for users and administrators.

| Page | Description |
|---|---|
| `index.html` | Main landing page |
| `login.html` | User login |
| `register.html` | Account registration |
| `dashboard.html` | User dashboard |
| `settings.html` | Account settings |
| `store.html` | Subscription store |
| `wallet.html` | Coin wallet |
| `download.html` | Download page |
| `admin.html` | Administration panel |
| `forgot-password.html` | Password recovery |
| `reset-password.html` | Password reset |
| `privacy.html` | Privacy information |
| `terms.html` | Terms and conditions |

### Frontend Structure

**CSS:** `website/css/`

Contains the individual stylesheets for the website.

**JavaScript:** `website/js/`

Contains the frontend logic for authentication, dashboard functionality, settings and password recovery.

**Images:** `website/img/`

Contains website logos and visual assets.

---

# 4. Offsets Dumper

**Location:** `offsetsdumper/`

The repository includes a separate executable called:

`cs2-dumper.exe`

This utility is intended for retrieving CS2-related offsets and generated definitions.

The resulting offset information can be used when maintaining the external application's offset definitions.

The dumper is distributed as a standalone executable in this repository. Its source code is not included in the current project structure.

---

# Communication Between Projects

The different components have separate responsibilities but work together.

```text
                 ┌──────────────────────┐
                 │    NexusClient       │
                 │      Website        │
                 └──────────┬───────────┘
                            │
                            ▼
                 ┌──────────────────────┐
                 │   Node.js Backend   │
                 │      server.js      │
                 └──────────┬───────────┘
                            │
              ┌─────────────┴─────────────┐
              │                           │
              ▼                           ▼
    ┌──────────────────┐       ┌──────────────────┐
    │  NexusClient     │       │  NexusClient     │
    │     Loader       │       │  Administration  │
    └────────┬─────────┘       └──────────────────┘
             │
             ▼
    ┌──────────────────┐
    │  Protected       │
    │  Downloads       │
    └────────┬─────────┘
             │
             ▼
    ┌──────────────────┐
    │ NexusClient      │
    │ External Source  │
    └──────────────────┘
```

### Typical User Workflow

1. A user creates an account through the website.
2. The backend stores the account information.
3. The user opens the NexusClient Loader.
4. The loader authenticates the user through the backend.
5. The backend validates the account and its HWID.
6. The loader retrieves the user's account and subscription information.
7. The loader can request protected application downloads.
8. The external application operates as a separate Windows application.

---

# Technologies

| Technology | Usage |
|---|---|
| C++ | Windows applications |
| C++20 | Modern C++ functionality |
| Win32 API | Windows integration |
| WinHTTP | HTTPS communication |
| DirectX 11 | Graphics rendering |
| Dear ImGui | External Source interface |
| GDI+ | Image processing |
| Node.js | Backend server |
| HTTP / JSON | API communication |
| HTML5 | Website structure |
| CSS3 | Website styling |
| JavaScript | Website functionality |
| Visual Studio 2022 | Development environment |

---

# Installation & Building

## Requirements

- Windows 10 or Windows 11
- Visual Studio 2022
- Desktop development with C++
- Windows SDK
- DirectX 11 development support
- Node.js and npm

## Build NexusClient External Source

1. Open `NexusClient External Source/NexusClient.sln`.
2. Select `Release`.
3. Select `x64`.
4. Build the solution.

## Build NexusClient Loader

1. Open `NexusClientLoader/nexusclientloader.sln`.
2. Select the desired build configuration.
3. Select `x64`.
4. Build the solution.

## Start the Backend

Navigate to the server directory:

```bash
cd NexusClientServer
```

Install dependencies:

```bash
npm install
```

Configure the required environment variables and server credentials.

Start the backend:

```bash
npm start
```

The backend uses port `6767` by default.

---

## Important Configuration

Before running the backend in a production environment, configure:

- Administrator credentials
- Email credentials
- HTTPS certificates and deployment configuration
- Server hostname
- Secure session handling

Do not publish real passwords, private keys, email credentials or production user databases in a public repository.

---

## Disclaimer

This project is provided for educational and research purposes.

NexusClient is an independent project and is not affiliated with, endorsed by or associated with Valve Corporation.

Counter-Strike 2 is a trademark of Valve Corporation.

Use of third-party software in online games may violate the applicable terms of service.

Users are responsible for complying with applicable laws and third-party terms.

---

## License

No open-source license has currently been specified.

Unless a license is added to this repository, all rights are reserved by the respective copyright holders.

---

<p align="center">
  <b>NexusClient</b>
  <br>
  Windows Applications · Backend · Web Platform
</p>