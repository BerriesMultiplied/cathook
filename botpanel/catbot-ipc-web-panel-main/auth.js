const randomstring = require('randomstring');
const fs = require('fs');
const net = require('net');

function parse_ipv4(text)
{
    const parts = String(text).split('.');
    if (parts.length !== 4)
        return null;

    const bytes = [];
    for (const part of parts)
    {
        if (!/^[0-9]+$/.test(part))
            return null;

        const value = Number.parseInt(part, 10);
        if (!Number.isInteger(value) || value < 0 || value > 255)
            return null;

        bytes.push(value);
    }

    return bytes;
}

function is_private_ipv4(text)
{
    const bytes = parse_ipv4(text);
    if (!bytes)
        return false;

    if (bytes[0] === 10)
        return true;

    if (bytes[0] === 172 && bytes[1] >= 16 && bytes[1] <= 31)
        return true;

    if (bytes[0] === 192 && bytes[1] === 168)
        return true;

    return bytes[0] === 127;
}

function normalize_request_ip(ip)
{
    const text = String(ip || '').trim().toLowerCase();
    if (text.startsWith('::ffff:'))
        return text.slice(7);

    return text;
}

function is_lan_ip(ip)
{
    const text = normalize_request_ip(ip);
    if (is_private_ipv4(text))
        return true;

    if (text === '::1')
        return true;

    return net.isIP(text) === 6 && (text.startsWith('fc') || text.startsWith('fd') || text.startsWith('fe8') || text.startsWith('fe9') || text.startsWith('fea') || text.startsWith('feb'));
}

class SimpleAuth
{
    constructor(app)
    {
        this.password = String(process.env.CAT_WEB_PASSWORD || randomstring.generate(12)).trim();
        this.apikey = randomstring.generate(64);

        app.post('/api/auth', this.handleLogin.bind(this));
        app.use(this.middleware.bind(this));
    }
    middleware(req, res, next)
    {
        if (!req.session.auth && is_lan_ip(req.ip))
        {
            console.log(`Auth: ${req.ip} by LAN`);
            req.session.auth = 1;
        }

        if (req.url.indexOf('api') < 0 || req.session.auth)
        {
            next();
            return;
        }

        if (req.query.key)
        {
            if (req.query.key === this.apikey)
            {
                console.log(`Auth: ${req.ip} by apikey`);
                req.session.auth = 1;
            }
            else
            {
                console.log(`Auth fail: ${req.ip} by invalid apikey ${req.query.key}`)
            }
        }

        if (!req.session.auth)
        {
            res.status(403).end('Not authorized');
            return;
        }
        next();
    }
    handleLogin(req, res)
    {
        const submitted_password = String(req.body.password || '').trim();
        if (submitted_password === this.password)
        {
            console.log(`Auth: ${req.ip} by password ${submitted_password}`);
            req.session.auth = 1;
            res.status(200).json({ ok: true });
        }
        else
        {
            console.log(`Auth fail: ${req.ip} by password ${submitted_password}`);
            res.status(403).json({ ok: false, error: 'invalid password' });
        }
    }
    storeAPIKey(path)
    {
        fs.writeFileSync(path, this.apikey);
    }
}

module.exports = SimpleAuth;
