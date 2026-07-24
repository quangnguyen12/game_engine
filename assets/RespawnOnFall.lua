-- =============================================
--  RespawnOnFall.lua
--  Tự động hồi sinh (Respawn) object về vị trí xuất phát nếu bị rơi khỏi bản đồ
-- =============================================

local RespawnOnFall = {}

function RespawnOnFall:onStart(obj)
    self.fallThreshold = -10.0
    self.spawnPos = { x = obj.position.x, y = obj.position.y + 2.0, z = obj.position.z }
    print("[Lua] RespawnOnFall initialized for: " .. obj.name)
end

function RespawnOnFall:onUpdate(obj, dt)
    -- Kiểm tra nếu vật thể rơi sâu hơn ngưỡng âm (-10.0)
    if obj.position.y < self.fallThreshold then
        print("[Lua] " .. obj.name .. " fell out of bounds! Respawning...")

        -- Đặt lại vị trí về điểm xuất phát
        obj.position.x = self.spawnPos.x
        obj.position.y = self.spawnPos.y
        obj.position.z = self.spawnPos.z

        -- Triệt tiêu toàn bộ vận tốc rơi
        obj.velocity.x = 0.0
        obj.velocity.y = 0.0
        obj.velocity.z = 0.0
    end
end

return RespawnOnFall
