const fs = require('fs');
const path = require('path');

const args = process.argv.slice(2);
const cronTitle = args[0];
const cronPeriod = args[1];
const scriptPath = args[2];

const fullPath = path.resolve(scriptPath);
const worker = require(fullPath);

if (worker.default && typeof worker.default.scheduled === 'function') {
    const controller = {
        cron: cronPeriod,
        type: "scheduled",
        scheduledTime: Date.now()
    };

    Promise.resolve(worker.default.scheduled(controller))
        .then(() => process.exit(0))
        .catch((err) => {
            console.error(err);
            process.exit(1);
        });
} else {
    console.error("Worker does not export a default object with a scheduled() method");
    process.exit(1);
}
