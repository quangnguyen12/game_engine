-- =============================================
--  GoldCollectible.lua
--  Logic thu thập vàng: Xoay tròn, kiểm tra va chạm với Player, cộng điểm và dịch chuyển ngẫu nhiên
-- =============================================

local GoldCollectible = {}

function GoldCollectible:onStart(obj)
    self.pickupRadius = 0.6
    print("[Lua] GoldCollectible initialized for: " .. obj.name)
end

function GoldCollectible:onUpdate(obj, dt)
    -- Xoay khối vàng liên tục tạo hiệu ứng đẹp mắt
    obj.rotation.y = obj.rotation.y + 90.0 * dt

    -- Lấy vị trí hiện tại của Player
    local playerPos = Game.getPlayerPosition()
    
    -- Tính khoảng cách giữa Player và Vàng
    local dx = obj.position.x - playerPos.x
    local dy = obj.position.y - playerPos.y
    local dz = obj.position.z - playerPos.z
    local dist = math.sqrt(dx * dx + dy * dy + dz * dz)

    -- Kiểm tra nếu Player chạm vào khoảng cách thu thập
    if dist < self.pickupRadius then
        print("[Lua] Gold Collected! +1 Score")

        -- Gọi hàm C++ API để cộng điểm trên HUD
        Game.addScore(1)

        -- Dịch chuyển đồng vàng sang tọa độ ngẫu nhiên mới
        obj.position.x = math.random(-30, 30) / 10.0
        obj.position.y = -1.2
        obj.position.z = math.random(-30, 30) / 10.0
    end
end

return GoldCollectible
