# Emot

`emot/motion_event` will publish 0 or 1 based on the PIN sendor (with debounce 50ms)
Send 1/0 to `emot/control` will trigger the relay

All other advanced logic such as rate-limiting will be done on host node-red machine