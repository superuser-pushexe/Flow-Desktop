project('flow-desktop', 'c')

# Dependencies
xcb_dep = dependency('xcb')
gtk_dep = dependency('gtk4')
gio_dep = dependency('gio-2.0')
sourceview_dep = dependency('gtksourceview-5')
vte_dep = dependency('vte-2.91')
git2_dep = dependency('libgit2')

# Executables
executable('flow',
           'flow.c',
           dependencies: [xcb_dep, gio_dep],
           install: true)

executable('flow-settings',
           'flow-settings.c',
           dependencies: [gtk_dep, gio_dep],
           install: true)

executable('flow-builder',
           'flow-builder.c',
           dependencies: [gtk_dep, sourceview_dep, vte_dep, git2_dep],
           install: true)
