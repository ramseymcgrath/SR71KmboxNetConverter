import unittest
import time
import kmNet  # This module must expose init, move, monitor, isdown_left, isdown_right,
             # isdown_middle, isdown_side1, isdown_side2, isdown_keyboard, mask_keyboard, and lcd_picture.

class TestKmNet(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        # Connect to the KMBox. For testing, we use these production values.
        kmNet.init('192.168.2.188', '8320', '24875054')

    def test_mouse_move_speed(self):
        # Measure time for 10000 mouse movement calls.
        cnt = 10000
        t1 = time.time()
        while cnt > 0:
            kmNet.move(0, 10)
            cnt -= 1
            kmNet.move(0, -10)
            cnt -= 1
        t2 = time.time()
        elapsed_ms = (t2 - t1) * 1000
        print(f'10000 calls took {elapsed_ms:.2f} ms')
        # Test passes if no exceptions occur.

    def test_mouse_monitoring(self):
        # Enable physical mouse monitoring.
        kmNet.init('192.168.2.188', '12545', 'F101383B')
        kmNet.monitor(10000)
        # Run a few iterations (0.5 sec each) and verify that, in absence of physical input,
        # the button states are reported as not pressed.
        for _ in range(5):
            self.assertFalse(kmNet.isdown_left(), "Left button should not be down")
            self.assertFalse(kmNet.isdown_right(), "Right button should not be down")
            self.assertFalse(kmNet.isdown_middle(), "Middle button should not be down")
            self.assertFalse(kmNet.isdown_side1(), "Side1 button should not be down")
            self.assertFalse(kmNet.isdown_side2(), "Side2 button should not be down")
            time.sleep(0.5)

    def test_keyboard_monitoring(self):
        # Enable keyboard monitoring.
        kmNet.init('192.168.2.188', '12545', 'F101383B')
        kmNet.monitor(10000)
        # Check over several iterations that the 'A' key (keycode 4) is not detected as pressed.
        for _ in range(5):
            self.assertEqual(kmNet.isdown_keyboard(4), 0, "A key should not be down")
            time.sleep(0.5)

    def test_mask_keyboard(self):
        # Enable monitoring and then mask the A key.
        kmNet.init('192.168.2.188', '12545', 'F101383B')
        kmNet.monitor(10000)
        kmNet.mask_keyboard(4)
        # Check that masking does not cause errors and that the key remains unpressed.
        for _ in range(5):
            self.assertEqual(kmNet.isdown_keyboard(4), 0, "A key should be masked and not report as down")
            time.sleep(0.5)

    @unittest.skip("LCD picture test is known to have issues")
    def test_lcd_picture(self):
        # Prepare a dummy image buffer (128 x 80 bytes).
        pic = bytearray(128 * 80)
        kmNet.lcd_picture(pic)
        # No assertions; this test is for demonstration.

if __name__ == '__main__':
    unittest.main()
