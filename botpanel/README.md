# Cat botpanel setup

This botpanel is bundled inside the main Cat repository. Do not clone separate
`catbot-ipc-server`, `catbot-ipc-web-panel`, or `cathook` repositories.

```sh
git clone https://github.com/pupnoodle/cathook
cd cathook
./setup.sh
```

Next, edit `botpanel/accounts.txt` and add bot accounts in this format:

USERNAME:PASSWORD
USERNAME:PASSWORD
USERNAME:PASSWORD

# Cat botpanel launcher

`./botpanel/start` starts the headless bot display with xpra by default.

- Default display: `:100`
- Override display: `CAT_XPRA_DISPLAY=:101 ./botpanel/start`
- Legacy override still accepted: `CAT_XVFB_DISPLAY=:101 ./botpanel/start`
- Use an existing desktop display instead: `CAT_VISIBLE_WINDOWS=1 ./botpanel/start`
- After game IPC has stayed connected for 10 seconds, the panel freezes the main `steamwebhelper` in that bot's Steam process tree and kills its child helper processes.
- Disable helper cleanup: `CAT_STEAMWEBHELPER_CLEANUP=0 ./botpanel/start`
- Override helper cleanup delay: `CAT_STEAMWEBHELPER_CLEANUP_SECONDS=15 ./botpanel/start`

`./botpanel/stop` stops the matching xpra display unless `CAT_VISIBLE_WINDOWS=1` is set.

`./botpanel/update` updates this single repository, installs dependencies,
builds Cat default/textmode libraries, builds the bundled IPC server, installs
web panel npm dependencies, and refreshes navmeshes when TF2 is installed.
