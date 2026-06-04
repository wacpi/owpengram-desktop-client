# 💻 OwpenGram Desktop

**Desktop client for OwpenGram server - Telegram-compatible messaging.**

<img width="884" height="707" alt="image" src="https://github.com/user-attachments/assets/7d615ed3-3dc8-474a-a420-17544245a170" />

---

## ✨ What is OwpenGram Desktop?

OwpenGram Desktop is a desktop messaging client compatible with the [OwpenGram Server](https://github.com/owpengram/owpengram-server). Based on Telegram Desktop, it provides a familiar interface for connecting to your own self-hosted messaging server.

## 🔗 Compatibility

This client is designed to work with the [OwpenGram Server](https://github.com/owpengram/owpengram-server). Simply configure the client to connect to your OwpenGram server instance and start messaging!

## ⚙️ Server Configuration

To change the server address, replace all instances of `XXX.XXX.XXX.XXX` in the code with your server's IP address or domain name.

## 📚 Build Instructions & Documentation

### Interactive build (Windows)

Double-click or run from any terminal:

```bat
build-desktop.cmd
```

The script will guide you through server IP, API credentials, submodules, `prepare`, `configure`, and MSBuild. Settings are saved to `.owpengram-build.local.json` (gitignored).

Requirements: **Visual Studio 2022** (C++ x64), **Python 3.10**, **Git**.

For manual steps and other platforms, see the original [Telegram Desktop build docs](https://github.com/telegramdesktop/tdesktop) and `docs/building-win-x64.md` in this repository.

## 💬 Community

- 📢 **Telegram Channel:** [@owpengram](https://t.me/owpengram)
- 💬 **Telegram Chat:** [Join the discussion](https://t.me/+sVB6Ymv70jEwNTAy)

## 📄 License

[GPLv3 with OpenSSL exception](LICENSE)

---

## ⭐ Give us a Star!

If OwpenGram Desktop helps you, consider giving us a star on GitHub!
