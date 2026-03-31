# Contributing to KWazzup

Thank you for taking the time to contribute!

## Reporting bugs

Use the [GitHub issue tracker](../../issues). Please include:

- KWazzup version
- KDE Plasma and Qt versions (`plasmashell --version`, `qtdiag | head`)
- Steps to reproduce
- Expected vs. actual behaviour
- Relevant output from `journalctl --user -f` or the terminal

## Suggesting features

Open an issue with the `enhancement` label. Describe the use-case clearly.

## Submitting code

1. Fork the repository and create a branch off `main`.
2. Follow the existing code style (C++17, Qt/KDE conventions, `clang-format` with the project's style).
3. Keep commits focused; write meaningful commit messages.
4. Make sure the project still builds cleanly (`cmake --build build`).
5. Open a pull request and fill in the template.

## Translations

KWazzup uses KDE's i18n infrastructure (`KLocalizedString` / `i18n()`). Translations can be submitted via [KDE's Weblate](https://l10n.kde.org/) once the project is registered there. In the meantime, `.po` files in `po/` are welcome.

## Code of Conduct

Be respectful. We follow the [KDE Code of Conduct](https://kde.org/code-of-conduct/).
