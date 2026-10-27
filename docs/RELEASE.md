## v1.0.0

Линейные поддержки теперь работают как постоянный инструмент: выберите его один раз и ставьте ряды последовательными парами точек. Выбор штатного инструмента возвращает обычное управление.

- Панель под автоподдержками; её можно свернуть, сохранив активный инструмент.
- Шаг ряда, включение последней точки, параметры штатных поддержек.
- Контакты на высоких выступах и в углублениях; пропуски там, где нет подходящей поверхности или CHITUBOX отклоняет поддержку.
- Отмена/повтор целого ряда, управление Escape и Ctrl+Shift+L.
- Ctrl + тачпад вращает камеру; прочие жесты сохраняют штатное поведение.
- Установщик без sudo и компиляции, отдельный ярлык, проверка совместимости и контрольной суммы загрузки.

**Совместимость:** native CHITUBOX Basic 2.3.1 Linux x86_64 с проверенными библиотеками, glibc 2.38+, Python 3.9+. Проверено на Arch Linux / Hyprland / XWayland. Другие сборки и ОС пока не поддерживаются. Панель расширения на русском языке.

```bash
curl -fsSL https://raw.githubusercontent.com/bibibobo479/Chitubox_extension/main/install.py | python3 -
```

Для ручной установки скачайте архив и SHA256SUMS, проверьте `sha256sum -c SHA256SUMS`, распакуйте и запустите `python3 install.py`.

**English:** Persistent two-click support rows, collapsible sidebar, surface projection across high protrusions, grouped undo/redo, and Ctrl-touchpad camera orbit. Install with the command above, then launch **CHITUBOX + Extensions**. Requires the matching native Basic 2.3.1 Linux x86_64 host and glibc 2.38+. See the English README for usage and limitations.

MIT applies to original extension code. This is an independent project, not endorsed by CHITUBOX / CBD-Tech. No CHITUBOX or Qt libraries are bundled. See LICENSE and NOTICE.md for rights and third-party notices.
