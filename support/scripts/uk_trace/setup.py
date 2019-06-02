from setuptools import setup

setup(
    name='uk_trace',
    version='0.1',
    scripts=['trace.py'],
    install_requires=[
        'Click', 'tabulate',
    ],
    entry_points='''
        [console_scripts]
        uk-trace=trace:cli
    ''',
)
