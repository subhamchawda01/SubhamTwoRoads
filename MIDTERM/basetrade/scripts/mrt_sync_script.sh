#!/bin/bash

rsync -ravz --timeout=60 10.0.1.46:/spare/local/MeanRevertPort/ /spare/local/MeanRevertPort/ --delete-after >/dev/null 2>&1
rsync -ravz --timeout=60 /spare/local/MeanRevertPort 10.23.102.55:/spare/local/ --delete-after >/dev/null 2>&1
rsync -ravz --timeout=60 /spare/local/MeanRevertPort 10.23.102.56:/spare/local/ --delete-after >/dev/null 2>&1
rsync -ravz --timeout=60 /spare/local/MeanRevertPort 10.23.196.53:/spare/local/ --delete-after >/dev/null 2>&1
rsync -ravz --timeout=60 /spare/local/MeanRevertPort 10.23.196.54:/spare/local/ --delete-after >/dev/null 2>&1
rsync -ravz --timeout=60 /spare/local/MeanRevertPort 10.23.82.55:/spare/local/ --delete-after >/dev/null 2>&1
rsync -ravz --timeout=60 /spare/local/MeanRevertPort 10.23.82.56:/spare/local/ --delete-after >/dev/null 2>&1
