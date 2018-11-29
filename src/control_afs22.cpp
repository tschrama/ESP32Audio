#include "control_afs22.h"
#include "Audio.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "driver/i2s.h"
#include "output_i2s.h"
#include "input_i2s.h"

bool AudioControlAFSTwoTwo::configured = false;

void AudioControlAFSTwoTwo::init(void)
{
    if(!configured)
    {
        int i2c_master_port = I2C_MASTER_NUM;
        i2c_config_t conf;
        conf.mode = I2C_MODE_MASTER;
        conf.sda_io_num = (gpio_num_t)I2C_MASTER_SDA_IO;
        conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
        conf.scl_io_num = (gpio_num_t)I2C_MASTER_SCL_IO;
        conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
        conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
        i2c_param_config((i2c_port_t)i2c_master_port, &conf);
        i2c_driver_install((i2c_port_t)i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);

        printf("Configuring PCM3060...\n");
        gpio_config_t io_conf;
        //disable interrupt
        io_conf.intr_type = (gpio_int_type_t)GPIO_PIN_INTR_DISABLE;
        //set as output mode
        io_conf.mode = GPIO_MODE_OUTPUT;
        //bit mask of the pins that you want to set,e.g.GPIO18/19
        io_conf.pin_bit_mask = GPIO_SEL_21;
        //disable pull-down mode
        io_conf.pull_down_en = (gpio_pulldown_t)0;
        //disable pull-up mode
        io_conf.pull_up_en = (gpio_pullup_t)0;
        //configure GPIO with the given settings
        gpio_config(&io_conf);

        gpio_set_level((gpio_num_t)PCM3060_PIN_RST, 1);         //Enable PCM3060
        printf("PCM3060 RST high\n");
        vTaskDelay(200/portTICK_RATE_MS);
        
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, PCM3060_ADR << 1 | WRITE_BIT, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, PCM3060_REG_64, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, 0xC0, ACK_CHECK_EN); //64
        i2c_master_write_byte(cmd, 0xFF, ACK_CHECK_EN); //65
        i2c_master_write_byte(cmd, 0xFF, ACK_CHECK_EN); //66
        i2c_master_write_byte(cmd, 0x80, ACK_CHECK_EN); //67
        i2c_master_write_byte(cmd, 0x00, ACK_CHECK_EN); //68
        i2c_master_write_byte(cmd, 0x00, ACK_CHECK_EN); //69
        i2c_master_write_byte(cmd, 0xD7, ACK_CHECK_EN); //70
        i2c_master_write_byte(cmd, 0xD7, ACK_CHECK_EN); //71
        i2c_master_write_byte(cmd, 0x00, ACK_CHECK_EN); //72
        i2c_master_write_byte(cmd, 0x08, ACK_CHECK_EN); //73
        i2c_master_stop(cmd);

        esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
        if (ret != ESP_OK) {
            printf("PCM3060 configuration error (%i).\n", ret);
            //return ret;
        }

        i2c_cmd_link_delete(cmd);
        printf("PCM3060 configured.\n");
        configured = true;
    }
}