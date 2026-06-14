#!/bin/bash

REPO_DIR="/home/rajat/languages/C"
cd "$REPO_DIR" || exit

echo "Starting auto-push script for $REPO_DIR..."
echo "Logging to auto_push.log"

while true; do
    git add .
    
    # Check if there's anything to commit
    if ! git diff --cached --quiet; then
        # Get list of changed files, up to 5 files
        CHANGED_FILES=$(git diff --cached --name-only | head -n 5 | paste -sd ", " -)
        
        # Check if there are more than 5 files
        FILE_COUNT=$(git diff --cached --name-only | wc -l)
        if [ "$FILE_COUNT" -gt 5 ]; then
            CHANGED_FILES="$CHANGED_FILES, and $(($FILE_COUNT - 5)) more"
        fi
        
        # Select a human-sounding verb for the commit message
        VERBS=("Updated" "Modified" "Tweaked" "Refined" "Adjusted" "Worked on")
        VERB=${VERBS[$RANDOM % ${#VERBS[@]}]}
        
        COMMIT_MSG="$VERB $CHANGED_FILES"
        
        git commit -m "$COMMIT_MSG"
        git push origin master
        
        echo "[$(date)] Pushed: $COMMIT_MSG" >> auto_push.log
    fi
    
    # Wait 5 minutes (300 seconds) before next check
    sleep 300
done
