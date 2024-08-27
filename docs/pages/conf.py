project = 'uPython'
copyright = '2024, Oliver Fabel'
author = 'Oliver Fabel'
release = '1.0.0'
language = 'en'

extensions = [
    'sphinx.ext.autodoc',
    'myst_parser'
]
source_suffix = {
    '.rst': 'restructuredtext',
    '.md': 'markdown'
}

templates_path = [
    'templates'
]
exclude_patterns = []
include_patterns = [
    '**'
]

html_theme = 'alabaster'
html_theme_options = {
    'extra_nav_links': {
        'Source Code': 'https://www.github.com/ofabel/mp-flipper',
        'Bugtracker': 'https://www.github.com/ofabel/mp-flipper/issues',
        'Releases': 'https://lab.flipper.net/apps/upython'

    }
}
html_static_path = [
    'static'
]
html_logo = 'assets/logo.png'
html_favicon = 'assets/favicon.png'