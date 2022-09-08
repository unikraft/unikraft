@ echo off

call mkdir logs

echo Install npm packages ......

call npm install

call npm install node-windows@1.0.0-beta.6 -g

call npm link node-windows

call node pccs.service.win
