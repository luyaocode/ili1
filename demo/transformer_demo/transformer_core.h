#ifndef TRANSFORMER_CORE_H
#define TRANSFORMER_CORE_H

#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include <unordered_map>

// 二维张量类型定义（序列维度×模型维度）
using Tensor2D = std::vector<std::vector<float>>;
// 三维张量类型定义（注意力头数×序列维度×序列维度）
using Tensor3D = std::vector<std::vector<std::vector<float>>>;
// 梯度字典（存储各层参数的梯度）
using GradDict = std::unordered_map<std::string, Tensor2D>;

// 工具函数命名空间
namespace TransformerUtils
{
    // 功能：随机初始化二维张量
    // 参数：rows-行数 cols-列数 scale-缩放系数（控制初始化范围）
    // 返回：初始化后的二维张量
    Tensor2D randomInit(int rows, int cols, float scale = 0.01f);

    // 新增：创建全0的二维张量（关键缺失函数）
    // 参数：rows-行数 cols-列数
    // 返回：全0初始化的二维张量
    Tensor2D zerosTensor(int rows, int cols);

    // 功能：矩阵乘法（仅支持二维张量）
    // 参数：a-左矩阵 b-右矩阵
    // 返回：a×b的结果矩阵
    Tensor2D matMul(const Tensor2D &a, const Tensor2D &b);

    // 功能：矩阵加法（支持广播，b可为1行/1列）
    // 参数：a-主矩阵 b-待加矩阵
    // 返回：a+b的结果矩阵
    Tensor2D matAdd(const Tensor2D &a, const Tensor2D &b);

    // 功能：层归一化（Transformer核心归一化方式）
    // 参数：x-输入矩阵 eps-防止除零的极小值
    // 返回：归一化后的矩阵
    Tensor2D layerNorm(const Tensor2D &x, float eps = 1e-5f);

    // 功能：矩阵转置
    // 参数：x-输入矩阵
    // 返回：转置后的矩阵
    Tensor2D transpose(const Tensor2D &x);

    // 功能：缩放点积注意力（多头注意力的核心计算单元）
    // 参数：q-查询矩阵 k-键矩阵 v-值矩阵 mask-掩码值（默认无掩码）
    // 返回：注意力加权后的输出矩阵
    Tensor2D scaledDotProductAttention(const Tensor2D &q, const Tensor2D &k, const Tensor2D &v, float mask = -1e9f);

    // 新增：矩阵逐元素乘法
    Tensor2D elementMul(const Tensor2D &a, const Tensor2D &b);

    // 新增：ReLU 反向传播
    Tensor2D reluBackward(const Tensor2D &d_out, const Tensor2D &x);

    // 新增：层归一化反向传播（简化版）
    Tensor2D layerNormBackward(const Tensor2D &d_out, const Tensor2D &x);

    // 新增：计算矩阵转置乘法（d_out × b^T）
    Tensor2D gradMatMul(const Tensor2D &d_out, const Tensor2D &b);

    // 新增：计算矩阵转置乘法（a^T × d_out）
    Tensor2D gradMatMulTrans(const Tensor2D &a, const Tensor2D &d_out);

    // 新增：Softmax正向函数（对每一行独立计算Softmax）
    Tensor2D softmax(const Tensor2D &x);

    // 新增：Softmax反向传播
    // 参数：d_out-Softmax输出的梯度，softmax_out-Softmax的前向输出
    // 返回：Softmax输入的梯度
    Tensor2D softmaxBackward(const Tensor2D &d_out, const Tensor2D &softmax_out);

    // 生成排序任务的输入和目标
    std::pair<Tensor2D, Tensor2D> createSortingTask(int seq_len, int d_model, int target_col = 0);

    // 计算排序准确率
    float calculateSortAccuracy(const Tensor2D &pred, const Tensor2D &target, int target_col = 0);

    // 提取序列的数值部分（第一个维度）
    std::vector<float> extractValues(const Tensor2D &x, int target_col);

    // 在TransformerUtils命名空间中新增
    // 生成批量排序任务数据
    std::vector<std::pair<Tensor2D, Tensor2D>>
        createBatchSortingTasks(int batch_size, int seq_len, int d_model, int target_col = 0);

    float sortLoss(const Tensor2D &pred, const Tensor2D &target, int target_col = 0);
    float batchSortLoss(const std::vector<Tensor2D> &preds, const std::vector<Tensor2D> &targets, int target_col = 0);

    // 批量计算排序准确率（平均）
    float batchSortAccuracy(const std::vector<Tensor2D> &preds, const std::vector<Tensor2D> &targets, int target_col = 0);

    // 新增：梯度归一化函数（C++11）
    Tensor2D normalizeGrad(const Tensor2D &grad, float max_norm = 1.0f);

    // 在TransformerUtils中新增：生成带排序索引的任务
    std::vector<std::pair<Tensor2D, Tensor2D>> createBatchSortingIndexTasks(int batch_size, int seq_len, int d_model, int target_col);

    Tensor2D constantInit(int rows, int cols, float val);

}  // namespace TransformerUtils

// 多头注意力层类
class MultiHeadAttention
{
public:
    // 构造函数
    // 参数：d_model-模型总维度 num_heads-注意力头数
    MultiHeadAttention(int d_model, int num_heads);

    // 功能：前向传播计算多头注意力
    // 参数：q-查询矩阵 k-键矩阵 v-值矩阵（自注意力时三者相同）
    // 返回：多头注意力输出矩阵
    Tensor2D forward(const Tensor2D &q, const Tensor2D &k, const Tensor2D &v);

    // 反向传播
    // 参数：d_out-输出梯度，返回：输入q/k/v的梯度 + 权重梯度
    std::tuple<Tensor2D, Tensor2D, Tensor2D, GradDict> backward(const Tensor2D &d_out);

    // 参数更新（SGD）
    void updateParams(float lr);

    // 功能：获取注意力权重（用于可视化）
    // 返回：三维注意力权重（头数×序列长×序列长）
    Tensor3D getAttentionWeights() const;

    // ========== 梯度累加核心接口（新增） ==========
    void resetAccumGrads();                  // 重置累加梯度
    void accumulateGrads();                  // 累加单样本梯度到累加梯度
    void averageAccumGrads(int batch_size);  // 平均累加梯度
    void updateParamsWithGrad(float lr);     // 用平均梯度更新参数
    // MultiHeadAttention新增
    void clearGrads();

private:
    int      m_dModel;       // 模型总维度
    int      m_numHeads;     // 注意力头数
    int      m_dK;           // 单个注意力头的维度（d_model/num_heads）
    Tensor2D m_Wq;           // 查询矩阵权重
    Tensor2D m_Wk;           // 键矩阵权重
    Tensor2D m_Wv;           // 值矩阵权重
    Tensor2D m_Wo;           // 输出投影权重
    Tensor3D m_attnWeights;  // 保存注意力权重（可视化用）

    // 新增：存储前向传播的中间结果（反向传播需要）
    Tensor2D m_q, m_k, m_v;
    Tensor3D m_qSplit, m_kSplit, m_vSplit;
    Tensor2D m_concat;

    GradDict m_grads;

    // 新增：前向传播中间值
    Tensor2D m_Q, m_K, m_V;  // 线性投影后的Q/K/V
    Tensor3D m_attnScores;   // 注意力分数（QK^T/√d_k）
    Tensor3D m_softmaxOut;   // Softmax后的注意力权重
    Tensor3D m_headOutputs;  // 每个头的输出
    Tensor2D m_finalOutput;  // 多头注意力最终输出

    // ========== 梯度累加核心成员（新增） ==========
    GradDict m_accumGrads;  // 累加梯度（存储批量中所有样本的梯度和）
};

// 前馈网络类（Transformer编码器的全连接层）
class FeedForwardNetwork
{
public:
    // 构造函数
    // 参数：d_model-模型维度 d_ff-隐藏层维度
    FeedForwardNetwork(int d_model, int d_ff);

    // 功能：前向传播
    // 参数：x-输入矩阵
    // 返回：前馈网络输出
    Tensor2D forward(const Tensor2D &x);

    // 反向传播
    // 参数：d_out-输出梯度，返回：输入梯度 + 权重梯度
    std::tuple<Tensor2D, GradDict> backward(const Tensor2D &d_out);

    // 参数更新（SGD）
    void updateParams(float lr);

    // ========== 梯度累加核心接口（新增） ==========
    void resetAccumGrads();                  // 重置累加梯度
    void accumulateGrads();                  // 累加单样本梯度到累加梯度
    void averageAccumGrads(int batch_size);  // 平均累加梯度
    void updateParamsWithGrad(float lr);     // 用平均梯度更新参数

    // FeedForwardNetwork新增
    void clearGrads();

private:
    Tensor2D m_W1;  // 第一层权重
    Tensor2D m_b1;  // 第一层偏置
    Tensor2D m_W2;  // 第二层权重
    Tensor2D m_b2;  // 第二层偏置

    // 新增：前向中间结果
    Tensor2D m_x, m_fc1, m_fc1Relu;
    GradDict m_grads;

    // ========== 梯度累加核心成员（新增） ==========
    GradDict m_accumGrads;  // 累加梯度（存储批量中所有样本的梯度和）
};

// Transformer编码器层（单个编码层）
class EncoderLayer
{
public:
    // 构造函数
    // 参数：d_model-模型维度 num_heads-注意力头数 d_ff-前馈网络隐藏层维度
    EncoderLayer(int d_model, int num_heads, int d_ff);

    // 功能：编码器层前向传播
    // 参数：x-输入序列矩阵
    // 返回：编码后的序列矩阵
    Tensor2D forward(const Tensor2D &x);

    // 反向传播
    Tensor2D backward(const Tensor2D &d_out);

    // 参数更新
    void updateParams(float lr);

    // 功能：获取该层的注意力权重
    // 返回：三维注意力权重
    Tensor3D getAttentionWeights() const;

    // ========== 梯度累加核心接口（新增） ==========
    void resetAccumGrads();                  // 重置当前层的累加梯度
    void accumulateGrads();                  // 累加当前层的单样本梯度
    void averageAccumGrads(int batch_size);  // 平均当前层的累加梯度
    void updateParamsWithGrad(float lr);     // 用平均梯度更新当前层参数
    // EncoderLayer新增转发接口
    void clearGrads();

private:
    MultiHeadAttention *m_mha;    // 多头注意力模块
    FeedForwardNetwork *m_ffn;    // 前馈网络模块
    Tensor2D            m_norm1;  // 第一层归一化结果
    Tensor2D            m_norm2;  // 第二层归一化结果

    // 新增：前向中间结果
    Tensor2D m_x, m_attnOutput, m_norm1Out, m_ffnOutput;
    Tensor3D m_attnWeights;
};

// Transformer编码器（多层EncoderLayer堆叠）
class TransformerEncoder
{
public:
    // 构造函数
    // 参数：d_model-模型维度 num_heads-注意力头数 d_ff-前馈网络维度 num_layers-编码器层数
    TransformerEncoder(int d_model = 64, int num_heads = 4, int d_ff = 128, int num_layers = 2);

    // 析构函数（释放内存）
    ~TransformerEncoder();

    // 功能：编码器前向传播
    // 参数：x-输入序列矩阵
    // 返回：最终编码输出
    Tensor2D forward(const Tensor2D &x);

    // 反向传播
    Tensor2D backward(const Tensor2D &d_out);

    // 参数更新
    void updateParams(float lr);

    // 功能：获取最后一层编码器的注意力权重
    // 返回：三维注意力权重
    Tensor3D getLastLayerAttentionWeights() const;

    // ========== 梯度累加核心接口（新增） ==========
    // 重置所有编码器层的累加梯度
    void resetAllAccumGrads();
    // 累加所有编码器层的单样本梯度
    void accumulateAllGrads();
    // 对所有编码器层的累加梯度取平均
    void averageAllAccumGrads(int batch_size);
    // 使用平均后的梯度更新所有参数
    void updateParamsWithAvgGrad(float lr);
    // TransformerEncoder新增转发接口
    void clearAllGrads();

private:
    int                         m_dModel;         // 模型维度
    std::vector<EncoderLayer *> m_encoderLayers;  // 编码器层列表
};

#endif  // TRANSFORMER_CORE_H
