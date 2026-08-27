# Publishing Brushpad to GitHub

This working tree is not published yet. Do not invent a remote URL.

## Handoff steps

1. The human creates an **empty** GitHub repository (no README, no license,
   no `.gitignore`).
2. The human gives the agent two things:
   - `OWNER/REPO` — replace this placeholder with the real GitHub owner
     and repository name
   - authentication the agent may use (`gh` already logged in, or another
     agreed method)
3. The agent then, and only then:
   ```sh
   git remote add origin git@github.com:OWNER/REPO.git
   git push -u origin main
   ```
   Substitute the real `OWNER/REPO` from step 2. Do not guess it.

Until `OWNER/REPO` and auth are provided, keep committing locally. Do not
add a remote. Do not push.
