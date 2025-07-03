function buildLowRes()
	vim.cmd("make lowres")
end

function buildHiRes()
	vim.cmd("make hires")
end

function buildDebug()
	vim.cmd("make debug")
end

function launchSimLowResColor()
	buildLowRes()
	vim.fn.jobstart({ "cloudpilot-emu", "emu/m505.image", "-s", "emu/cloudpilotStartRelease.cp" })
end

function launchSimLowResBW()
	buildLowRes()
	vim.fn.jobstart({ "cloudpilot-emu", "emu/m100.image", "-s", "emu/cloudpilotStartRelease.cp" })
end

function launchSimDebug()
	buildDebug()
	vim.fn.jobstart({ "cloudpilot-emu", "emu/m505.image", "-s", "emu/cloudpilotStartDebug.cp" })
end

function launchSimHiRes()
	buildHiRes()
	vim.cmd(
		"botright split | terminal emu/createsd.sh artifacts/StarGrid_hires.prc && cp-uarm emu/uarm.session -s emu/sdcard.image"
	)
	--	vim.fn.jobstart({ "cp-uarm", "emu/uarm.session", "-s", "emu/sdcard.image" })
end

function installHiRes()
	buildHiRes()
	vim.cmd("!pilot-xfer -pusb: -i artifacts/StarGrid_hires.prc")
end

vim.keymap.set("n", "<leader>lc", launchSimLowResColor, { desc = "Cloudpilot lowres color" })
vim.keymap.set("n", "<leader>lb", launchSimLowResBW, { desc = "Cloudpilot lowres black white" })
vim.keymap.set("n", "<leader>ld", launchSimDebug, { desc = "Cloudpilot debug" })
vim.keymap.set("n", "<leader>lh", launchSimHiRes, { desc = "Cloudpilot hires" })
vim.keymap.set("n", "<leader>li", installHiRes, { desc = "Install HIRES to Palm" })
