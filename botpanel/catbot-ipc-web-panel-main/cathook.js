const child_process = require('child_process');
const EventEmitter = require('events');
const extend = require('extend');

const CATHOOK_ROOT = process.env.CATHOOK_ROOT || '/opt/cathook';
const CONSOLE_PATH = `${CATHOOK_ROOT}/ipc/bin/console`;
const IPC_COMMAND_TIMEOUT = Number.parseInt(process.env.CAT_IPC_COMMAND_TIMEOUT_SECONDS || '30', 10) * 1000;

class CathookConsole extends EventEmitter {
    constructor() {
        super();
        this.setMaxListeners(256);
        var self = this;
        this.init = false;
        this.next_cmdid = 1;
        this.process = child_process.spawn(CONSOLE_PATH);
        this.process.on('error', function (error) {
            self.init = false;
            console.log('[!] failed to start cathook console:', error.message);
            self.emit('exit');
        });
        this.process.on('exit', function (code) {
            self.init = false;
            self.emit('exit');
            console.log('[!] cathook console exited with code', code);
        });
        this.process.stdin.on('error', function (error) {
            console.log('[!] cathook console stdin error:', error.message);
        });
        var buff = '';
        this.process.stdout.on('data', function (data) {
            buff += data.toString();
            var newline_index = buff.indexOf('\n');
            while (newline_index !== -1) {
                const line = buff.slice(0, newline_index);
                buff = buff.slice(newline_index + 1);
                newline_index = buff.indexOf('\n');
                if (!line)
                    continue;

                try {
                    const clean_line = line.replace(/[\uFFFD\uFFFE\uFFFF]/g, '');
                    const parsed = JSON.parse(clean_line);
                    self.emit('data', parsed);
                } catch (e) {
                    console.log('Error parsing IPC data:', e.message);
                    console.log('Raw buffer length:', line.length);
                    self.emit('data', null);
                }
            }
        });
        this.on('data', function (data) {
            if (!data) return;
            if (data.init) {
                self.init = true;
                self.emit('init');
            };
        });
    }
    command(cmd, data, callback) {
        if (!this.process || !this.process.stdin || this.process.stdin.destroyed) {
            if (callback)
                callback({ status: 'error', error: 'cathook console is not running' });
            return;
        }

        data = extend({}, data || {}, { "command": cmd });
        let handler = null;
        let callback_timeout = null;
        if (callback) {
            const cmdid = String(this.next_cmdid++);
            data.cmdid = cmdid;
            handler = (response) => {
                if (!response || response.cmdid !== cmdid)
                    return;

                this.removeListener('data', handler);
                clearTimeout(callback_timeout);
                callback(response);
            };
            this.on('data', handler);
            callback_timeout = setTimeout(() => {
                this.removeListener('data', handler);
                callback({ status: 'error', error: 'cathook console command timed out' });
            }, IPC_COMMAND_TIMEOUT);
            if (callback_timeout.unref)
                callback_timeout.unref();
        }
        try {
            this.process.stdin.write(JSON.stringify(data) + '\n');
        } catch (error) {
            if (handler) {
                this.removeListener('data', handler);
                clearTimeout(callback_timeout);
            }
            if (callback)
                callback({ status: 'error', error: error.message });
            console.log('[!] cathook console write failed:', error.message);
        }
    }
    stop() {
        this.command("exit", {});
    }
}

module.exports = CathookConsole;
