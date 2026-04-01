export default {
  scheduled(controller) {
    console.log("Cron Job Fired!");
    console.log("Cron Expression:", controller.cron);
    console.log("Type:", controller.type);
    console.log("Scheduled Time:", new Date(controller.scheduledTime).toISOString());
  },
};
