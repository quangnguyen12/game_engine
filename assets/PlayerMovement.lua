-- =============================================
--  PlayerMovement.lua
--  Gắn vào bất kỳ object nào để điều khiển di chuyển
--  bằng phím IJKL (hoặc tùy chỉnh)
-- =============================================

local PlayerMovement = {}

function PlayerMovement:onStart(obj)
    -- Lưu vị trí ban đầu
    self.startPos = vec3.new(obj.position.x, obj.position.y, obj.position.z)
    self.speed = 2.0
    self.time = 0
    print("[Lua] PlayerMovement started on: " .. obj.name)
    print("[Lua] Start position: " .. obj.position.x .. ", " .. obj.position.y .. ", " .. obj.position.z)
end

function PlayerMovement:onUpdate(obj, dt)
    self.time = self.time + dt

    -- Di chuyển theo Sin wave trên trục Y (nhảy lên xuống nhẹ)
    local bounceHeight = 0.15
    local bounceSpeed = 3.0
    obj.position.y = self.startPos.y + math.sin(self.time * bounceSpeed) * bounceHeight

    -- Xoay object liên tục
    obj.rotation.y = obj.rotation.y + 45.0 * dt

    -- Di chuyển chậm theo hình tròn
    local radius = 1.5
    local circleSpeed = 0.5
    obj.position.x = self.startPos.x + math.cos(self.time * circleSpeed) * radius
    obj.position.z = self.startPos.z + math.sin(self.time * circleSpeed) * radius
end

return PlayerMovement
