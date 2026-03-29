exports.default = {
  scheduled(controller) {
    console.log("Cron triggered!");
    console.log("Cron pattern:", controller.cron);
    console.log("Type:", controller.type);
    console.log("Scheduled Time:", new Date(controller.scheduledTime).toISOString());
  },
};
