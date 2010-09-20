#!/usr/bin/env python

import os
from distutils.core import setup
from distutils.debug import DEBUG

setup(name='procman',
        version='0.0.1',
        description='procman',
        packages = [ 'bot_procman' ],
        package_dir = { '' : 'src' },
        )

