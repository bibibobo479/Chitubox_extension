# Copyright and third-party notices

## Extension source

Copyright (c) 2026 SSI. The extension's original source code is licensed under the MIT license in [LICENSE](LICENSE). The copyright holder is preserved from this repository's original license. Contributions are accepted under the same license.

## CHITUBOX

CHITUBOX and associated product names and trademarks belong to their respective owners. This is an independent interoperability project, not an official SDK plugin and not affiliated with, sponsored by, or endorsed by CHITUBOX or CBD-Tech.

No CHITUBOX executable, shared library, extracted QML, icon, vendor source, or installer is distributed in this repository or its release package. Users must obtain and license CHITUBOX separately. Symbol names, compatibility hashes and ABI layout information are used to identify and interoperate with the user's installed software.

The MIT license covers our code only. It does not amend CHITUBOX's license terms, grant rights to its trademarks, or establish that all uses of internal APIs are permitted in every jurisdiction. Review the terms accompanying your CHITUBOX installation and any applicable interoperability exceptions. No legal clearance from the vendor is claimed.

## Qt

The extensions use Qt Core, Gui, Widgets, Qml and Quick through dynamic linking. Qt is copyright The Qt Company Ltd. and other contributors, and is available under commercial and open-source licenses. This project uses the LGPLv3 route for the relevant modules. See https://doc.qt.io/qt-6/licensing.html and https://www.qt.io/development/open-source-lgpl-obligations .

Release packages do not contain Qt shared libraries. They use the Qt runtime already supplied with the user's separately installed CHITUBOX. Compiled extensions can contain inline/template material from Qt headers, which remains under the applicable Qt license; that material is not relicensed under MIT. Full LGPLv3 and GPLv3 texts accompany this release as LGPL-3.0.txt and GPL-3.0.txt.

Complete extension source and build instructions are provided in this repository, with matching release tags. You may modify and rebuild the extension against a compatible modified Qt runtime, replace the dynamically loaded Qt libraries, and reverse engineer the extension for debugging such modifications; this project imposes no restriction on those activities. ABI compatibility with the separately licensed host application is still required. Any terms applying to the vendor's own Qt distribution remain the vendor's responsibility.

Qt sources: https://download.qt.io/archive/qt/ . Consult your Qt provider's notices for the exact version and any patches. No modifications to Qt are distributed by this project.
