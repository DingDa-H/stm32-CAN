/**
 * @brief 		can测试
 * @version 	DingDa-H	
 * @data 		20261109	
 * @note 		优化分层架构，使其尽量符合AUTOSAR思想
 * @note 		用于分配不同的过滤器策略（全通、列表、屏蔽）以及接收/发送角色
 */
 
#ifndef __CAN_TEST_H
#define __CAN_TEST_H

#include <stdint.h>

/**
 * @brief   按索引发送测试帧表中的一帧
 * @param   ucIndex  要发送的帧在测试表中的索引（0 ~ 表长-1）
 * @note    若索引越界则不执行任何操作；发送后测试数据首字节会自动+1
 */
void vCanTestSendOneByIndex(uint8_t ucIndex);

/**
 * @brief   获取当前测试配置下的总帧数
 * @retval  当前测试帧表的条目数量
 */
uint8_t ucCanTestGetFrameNum(void);

#endif
