from pathlib import Path
import os
from typing import Any

project = "bf"
author = "Ben M. Andrew"
copyright = f"2026, {author}"
language = "en_GB"
# Sphinx HTML search uses the English stemmer code `en` for all English variants.
html_search_language = "en"
extensions = ["breathe"]
templates_path = ["_templates"]
exclude_patterns = []
html_theme = "furo"
html_static_path = ["_static"]
html_title = "bf Documentation"

html_theme_options: dict[str, Any] = {
    "source_repository": "https://github.com/benmandrew/bf/",
    "source_branch": "main",
    "sidebar_hide_name": False,
    "navigation_with_keys": True,
    "footer_icons": [
        {
            "name": "GitHub",
            "url": "https://github.com/benmandrew/CEGIW",
            "html": """
                <svg stroke="currentColor" fill="currentColor" stroke-width="0" viewBox="0 0 16 16">
                    <path d="M8 0C3.58 0 0 3.58 0 8c0 3.54 2.29 6.53 5.47 7.59.4.07.55-.17.55-.38 0-.19-.01-.82-.01-1.49-2.01.37-2.53-.49-2.69-.94-.09-.23-.48-.94-.82-1.13-.28-.15-.68-.52-.01-.53.63-.01 1.08.58 1.23.82.72 1.21 1.87.87 2.33.66.07-.52.28-.87.5-1.07-1.78-.2-3.64-.89-3.64-3.95 0-.87.31-1.59.82-2.15-.08-.2-.36-1.02.08-2.12 0 0 .67-.21 2.2.82a7.56 7.56 0 0 1 2-.27c.68 0 1.36.09 2 .27 1.53-1.04 2.2-.82 2.2-.82.44 1.1.16 1.92.08 2.12.51.56.82 1.27.82 2.15 0 3.07-1.87 3.75-3.65 3.95.29.25.54.73.54 1.48 0 1.07-.01 1.93-.01 2.2 0 .21.15.46.55.38A8.013 8.013 0 0 0 16 8c0-4.42-3.58-8-8-8z"></path>
                </svg>
            """,
            "class": "",
        }
    ],
}

default_xml_dir = Path(__file__).resolve().parent.parent / "build" / "docs" / "doxygen" / "xml"
breathe_projects = {
    "bf": os.environ.get("SPHINX_DOXYGEN_XML_DIR", str(default_xml_dir))
}
breathe_default_project = "bf"
