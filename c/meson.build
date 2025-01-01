project(
    'sserver',
    'c',
    version: '0.1',
    default_options: ['warning_level=3', 'c_std=c23', 'b_lto=true'],
)

thread_dep = dependency('threads')

executable(
    'sserver',
    'log.c',
    'socks.c',
    'server.c',
    'main.c',
    'http.c',
    'relay.c',
    install: true,
    dependencies: [
        thread_dep,
    ],
)
