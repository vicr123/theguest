TEMPLATE = subdirs

tools.subdir = spice-tools

app.subdir = application
app.depends = tools

SUBDIRS += \
    app \
    tools
