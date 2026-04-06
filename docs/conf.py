from pathlib import Path
import os

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

default_xml_dir = Path(__file__).resolve().parent.parent / "build" / "docs" / "doxygen" / "xml"
breathe_projects = {
    "bf": os.environ.get("SPHINX_DOXYGEN_XML_DIR", str(default_xml_dir))
}
breathe_default_project = "bf"
