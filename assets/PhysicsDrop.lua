-- =============================================
--  PhysicsDrop.lua  
--  Gắn vào object có RigidBody Component
--  Khi Play: đẩy object lên trời rồi để rơi tự do
-- =============================================

local PhysicsDrop = {}

function PhysicsDrop:onStart(obj)
    -- Đẩy object lên cao khi bắt đầu Play
    obj.position.y = obj.position.y + 5.0
    obj.velocity = vec3.new(0, 0, 0)
    self.timer = 0
    print("[Lua] PhysicsDrop: " .. obj.name .. " launched to Y=" .. obj.position.y)
end

function PhysicsDrop:onUpdate(obj, dt)
    self.timer = self.timer + dt

    -- Xoay nhẹ khi rơi
    obj.rotation.x = obj.rotation.x + 90.0 * dt
    obj.rotation.z = obj.rotation.z + 60.0 * dt
end

return PhysicsDrop
