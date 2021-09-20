-- Кнопки
local ConfigSignals = {
    "SetKV0",
    "SetKV1",
    "SetKV2",
    "SetKV3",
    "SetKV4",
    "SetKV5",
    "SetKV6",
    "R_Program1",
    "R_Program2",
    "KVT",
    "KU12",
    "KU7",
    "V2"
}
-- Контроллер машиниста
local KVPosByte = {
    [7] = 3,    --Х3
    [3] = 2,    --Х2
    [1] = 1,    --Х1
    [64] = 0,   -- 0
    [16] = -1,  --Т1
    [26] = -2,  --Т1а
    [50] = -3,  --Т2
}

local libFounded = UART
if not UART then
	if pcall(require,"uart") then
		print("UART Lib: Loaded!")
		print(Format("Version: %s",UART.Version()))
        libFounded = true
	else
		print("UART Lib not found")
	end
end
if not libFounded then return end

CH_Con = CH_Con or {}
function CH_Con:Initialize()
	if UART.GetCOMState() ~= -10 then
		UART.StopCOM()
	end
	self.inSignalsTrain = {}
    for i=1,#ConfigSignals do
        self.inSignalsTrain[i] = {ConfigSignals[i],0}
    end
	self.inBytesNum = math.ceil(#self.inSignalsTrain/8) + 1
	self.Connected = false
    self.LastUpdate = 0
end

function CH_Con:Connect(port)
	self:Initialize()
	local COMState = UART.StartCOM(port,74880)
	if COMState == 0 then
		print("UART Lib: Connected!")
		self.Connected = true
	else
		print(Format("UART Lib: connect failed. (%d)", COMState))
		self.Connected = false
	end
end

local KVPos,KVPosPin,inSignals,inBytes
function CH_Con:Update(train)
	inSignals = {}
    inBytes = UART.ReadByte(self.inBytesNum)
    if inBytes[0] ~= self.inBytesNum then return end
    self.LastUpdate = CurTime()
	for i=0,self.inBytesNum-1 do
		for pinNmb=0,7 do
			inSignals[8*i + pinNmb] = bit.band(0x01,bit.rshift(inBytes[i],pinNmb)) -- inSignals[8*i + pinNmb] = (inBytes[i] >> pinNmb) & 0x01
		end
	end
	KVPos = 0
	KVPosPin = 0
	for i=1,#self.inSignalsTrain do
		self.inSignalsTrain[i][2] = inSignals[i-1]
		local btn,val = train[self.inSignalsTrain[i][1]],self.inSignalsTrain[i][2]
		if btn then
			if btn.Value ~= val then
				btn:TriggerInput("Set",val)
			end
		else
			if self.inSignalsTrain[i][1]:find("SetKV") then
				KVPos = bit.bor(KVPos,bit.lshift(val,KVPosPin))
				KVPosPin = KVPosPin + 1
			end
		end
	end
    -- print(KVPos)
	KVPos = KVPosByte[KVPos] or train.KV.ControllerPosition
	if train.KV.ControllerPosition ~= KVPos then
        train.KV:TriggerInput("ControllerSet",KVPos)
    end
end

timer.Simple(1,function()
	local TrainEnt = scripted_ents.GetStored("gmod_subway_ezh3").t
	TrainEnt.Think_Old = TrainEnt.Think_Old or TrainEnt.Think
    
	function TrainEnt:Think()
		self.RetVal = self:Think_Old()
		if CH_Con.Connected and IsValid(self.DriverSeat:GetDriver()) then
			CH_Con:Update(self)
		end
		return self.RetVal
	end
end)

concommand.Add("pstart", function(ply,cmd,args) if not args[1] then print("Enter number of COM!") else print("Connecting, please wait...") CH_Con:Connect(args[1]) end end,nil,"Connect to Ezh 3 controller")
concommand.Add("pstop", function(ply,cmd,args) print("Ezh3 controller disconnected!") UART.StopCOM() CH_Con.Connected = false end,nil,"Disconnect Ezh 3 controller")