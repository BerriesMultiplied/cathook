const CathookConsole = require('./cathook');
const express = require('express');
const bodyparser = require('body-parser');
const path = require('path');
const { Forever } = require('./forever/app');
const fs = require('fs');
const stoppable = require("stoppable");

const PORT = Number.parseInt(process.env.CAT_IPC_PORT || '7655', 10);

const npid = require('npid');
try {
    const pid = npid.create('/tmp/ncat-cathook-webpanel.pid', true);
    pid.removeOnExit();
}
catch (error) {
    console.log(`Webpanel already running?`);
    process.exit(1);
}

const app = express();

const session = require('express-session');

app.use(session({
    secret: require('randomstring').generate(16),
    resave: false,
    saveUninitialized: false
}))

app.use(express.static(path.join(__dirname, "public")));
app.use(bodyparser.json());
app.use(bodyparser.urlencoded({ extended: true }));

const SimpleAuth = require('./auth');
const basicAuth = new SimpleAuth(app);
console.log('Login with password', basicAuth.password);
fs.writeFileSync('/tmp/cat-webpanel-password', basicAuth.password);

const cc = new CathookConsole();

var forever = new Forever(app, cc);

cc.once('init', () => {
    cc.command('connect');
});
cc.on('exit', () => { });

app.post('/api/direct/:command', function (req, res) {
    cc.command(req.params.command, req.body, function (data) {
        res.send(data);
    });
});

var server = app.listen(PORT, function () {
    console.log("Listening on port", PORT);
});
stoppable(server, 0);


const sauce_watcher = fs.watch('/tmp', (eventType, filename) => {
    if (filename && filename.startsWith("source_engine") && filename.endsWith(".lock") && fs.existsSync(`/tmp/${filename}`))
    {
        try
        {
            fs.unlinkSync(`/tmp/${filename}`);
        }
        catch (err)
        {

        }
    }
})

process.on("SIGINT", function () {
    server.stop();
    forever.stop();
    cc.stop();
    sauce_watcher.close();
});
