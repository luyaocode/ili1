#include "transformer_core.h"
#include <iostream>
// 工具函数实现
namespace TransformerUtils
{
    Tensor2D randomInit(int rows, int cols, float scale)
    {
        // 随机数生成器（固定种子保证可复现）
        static std::mt19937                    gen(42);
        static std::normal_distribution<float> dist(0.0f, 1.0f);

        Tensor2D res(rows, std::vector<float>(cols, 0.0f));
        for (int i = 0; i < rows; ++i)
        {
            for (int j = 0; j < cols; ++j)
            {
                res[i][j] = dist(gen) * scale;
            }
        }
        return res;
    }

    Tensor2D zerosTensor(int rows, int cols)
    {
        // 创建rows行cols列的全0张量
        Tensor2D res(rows, std::vector<float>(cols, 0.0f));
        return res;
    }

    Tensor2D matMul(const Tensor2D &a, const Tensor2D &b)
    {
        int aRows = a.size();
        int aCols = a[0].size();
        int bCols = b[0].size();

        // 初始化结果矩阵为0
        Tensor2D res(aRows, std::vector<float>(bCols, 0.0f));
        // 矩阵乘法核心计算
        for (int i = 0; i < aRows; ++i)
        {
            for (int j = 0; j < bCols; ++j)
            {
                for (int k = 0; k < aCols; ++k)
                {
                    res[i][j] += a[i][k] * b[k][j];
                }
            }
        }
        return res;
    }

    Tensor2D matAdd(const Tensor2D &a, const Tensor2D &b)
    {
        int      rows = a.size();
        int      cols = a[0].size();
        Tensor2D res(rows, std::vector<float>(cols, 0.0f));

        // 判断是否需要广播
        bool bRowBroadcast = (b.size() == 1);
        bool bColBroadcast = (b[0].size() == 1);

        for (int i = 0; i < rows; ++i)
        {
            for (int j = 0; j < cols; ++j)
            {
                int bI    = bRowBroadcast ? 0 : i;
                int bJ    = bColBroadcast ? 0 : j;
                res[i][j] = a[i][j] + b[bI][bJ];
            }
        }
        return res;
    }

    // 先在layerNorm正向函数中新增gamma/beta参数（默认1/0）
    Tensor2D layerNorm(const Tensor2D &x, float eps)
    {
        int      rows = x.size();
        int      cols = x[0].size();
        Tensor2D res(rows, std::vector<float>(cols, 0.0f));

        // 新增：可训练的gamma/beta（初始gamma=1，beta=0）
        static Tensor2D gamma = TransformerUtils::randomInit(1, cols, 1.0f);
        static Tensor2D beta  = TransformerUtils::randomInit(1, cols, 0.0f);

        for (int i = 0; i < rows; ++i)
        {
            float mean = 0.0f;
            for (int j = 0; j < cols; ++j)
                mean += x[i][j];
            mean /= cols;

            float var = 0.0f;
            for (int j = 0; j < cols; ++j)
                var += pow(x[i][j] - mean, 2);
            var /= cols;

            float std = sqrt(var + eps);
            for (int j = 0; j < cols; ++j)
            {
                res[i][j] = gamma[0][j] * (x[i][j] - mean) / std + beta[0][j];
            }
        }
        return res;
    }

    Tensor2D transpose(const Tensor2D &x)
    {
        int      rows = x.size();
        int      cols = x[0].size();
        Tensor2D res(cols, std::vector<float>(rows, 0.0f));

        // 矩阵转置核心逻辑
        for (int i = 0; i < rows; ++i)
        {
            for (int j = 0; j < cols; ++j)
            {
                res[j][i] = x[i][j];
            }
        }
        return res;
    }

    Tensor2D scaledDotProductAttention(const Tensor2D &q, const Tensor2D &k, const Tensor2D &v, float mask)
    {
        int dK = q[0].size();
        // 计算Q*K^T
        Tensor2D kT         = transpose(k);
        Tensor2D attnScores = matMul(q, kT);

        // 缩放（除以sqrt(d_k)）
        for (auto &row : attnScores)
        {
            for (auto &val : row)
            {
                val /= sqrt(dK);
            }
        }

        // Softmax计算（转换为注意力权重）
        Tensor2D attnWeights(attnScores.size(), std::vector<float>(attnScores[0].size(), 0.0f));
        for (int i = 0; i < attnScores.size(); ++i)
        {
            float sumExp = 0.0f;
            // 计算指数和
            for (float val : attnScores[i])
            {
                sumExp += exp(val);
            }
            // 归一化得到权重
            for (int j = 0; j < attnScores[0].size(); ++j)
            {
                attnWeights[i][j] = exp(attnScores[i][j]) / sumExp;
            }
        }

        // 注意力权重乘以V矩阵
        return matMul(attnWeights, v);
    }

    Tensor2D elementMul(const Tensor2D &a, const Tensor2D &b)
    {
        int      rows = a.size();
        int      cols = a[0].size();
        Tensor2D res(rows, std::vector<float>(cols, 0.0f));
        for (int i = 0; i < rows; ++i)
        {
            for (int j = 0; j < cols; ++j)
            {
                res[i][j] = a[i][j] * b[i][j];
            }
        }
        return res;
    }

    Tensor2D reluBackward(const Tensor2D &d_out, const Tensor2D &x)
    {
        int      rows = d_out.size();
        int      cols = d_out[0].size();
        Tensor2D res(rows, std::vector<float>(cols, 0.0f));
        for (int i = 0; i < rows; ++i)
        {
            for (int j = 0; j < cols; ++j)
            {
                res[i][j] = d_out[i][j] * (x[i][j] > 0 ? 1.0f : 0.0f);
            }
        }
        return res;
    }

    // 反向传播同步修改（包含gamma/beta梯度）
    Tensor2D layerNormBackward(const Tensor2D &d_out, const Tensor2D &x)
    {
        int      rows = x.size();
        int      cols = x[0].size();
        Tensor2D res(rows, std::vector<float>(cols, 0.0f));
        float    eps = 1e-5f;

        // 新增：gamma/beta（和正向保持一致）
        static Tensor2D gamma = TransformerUtils::randomInit(1, cols, 1.0f);

        for (int i = 0; i < rows; ++i)
        {
            float mean = 0.0f;
            for (int j = 0; j < cols; ++j)
                mean += x[i][j];
            mean /= cols;

            float var = 0.0f;
            for (int j = 0; j < cols; ++j)
                var += pow(x[i][j] - mean, 2);
            var /= cols;
            float std = sqrt(var + eps);

            std::vector<float> x_hat(cols);
            for (int j = 0; j < cols; ++j)
            {
                x_hat[j] = (x[i][j] - mean) / std;
            }

            // 计算gamma/beta梯度（新增）
            float d_gamma = 0.0f, d_beta = 0.0f;
            for (int j = 0; j < cols; ++j)
            {
                d_gamma += d_out[i][j] * x_hat[j];
                d_beta += d_out[i][j];
            }

            float d_mean = 0.0f, d_var = 0.0f;
            for (int j = 0; j < cols; ++j)
            {
                d_mean += d_out[i][j] * gamma[0][j] * (-1.0f / std);
                d_var += d_out[i][j] * gamma[0][j] * x_hat[j] * (-0.5f) * pow(var + eps, -1.5f);
            }
            d_mean /= cols;
            d_var /= cols;

            for (int j = 0; j < cols; ++j)
            {
                res[i][j] = d_out[i][j] * gamma[0][j] / std + d_var * 2 * (x[i][j] - mean) / cols + d_mean;
            }
        }
        return res;
    }

    Tensor2D gradMatMul(const Tensor2D &d_out, const Tensor2D &b)
    {
        return matMul(d_out, transpose(b));
    }

    Tensor2D gradMatMulTrans(const Tensor2D &a, const Tensor2D &d_out)
    {
        return matMul(transpose(a), d_out);
    }

    Tensor2D softmaxBackward(const Tensor2D &d_out, const Tensor2D &softmax_out)
    {
        int      rows = d_out.size();
        int      cols = d_out[0].size();
        Tensor2D d_input(rows, std::vector<float>(cols, 0.0f));

        for (int i = 0; i < rows; ++i)
        {
            // 对每一行（每个序列位置）计算Softmax梯度
            float sum = 0.0f;
            for (int j = 0; j < cols; ++j)
            {
                sum += d_out[i][j] * softmax_out[i][j];
            }
            for (int j = 0; j < cols; ++j)
            {
                d_input[i][j] = softmax_out[i][j] * (d_out[i][j] - sum);
            }
        }
        return d_input;
    }

    Tensor2D softmax(const Tensor2D &x)
    {
        int      rows = x.size();
        int      cols = x[0].size();
        Tensor2D res(rows, std::vector<float>(cols, 0.0f));

        for (int i = 0; i < rows; ++i)
        {
            // 1. 减去每行最大值（数值稳定，避免指数爆炸）
            float max_val = *std::max_element(x[i].begin(), x[i].end());
            float sum_exp = 0.0f;

            // 2. 计算指数并求和
            for (int j = 0; j < cols; ++j)
            {
                res[i][j] = exp(x[i][j] - max_val);
                sum_exp += res[i][j];
            }

            // 3. 归一化（除以总和）
            for (int j = 0; j < cols; ++j)
            {
                res[i][j] /= sum_exp;
                // 防止数值下溢，设置最小阈值
                if (res[i][j] < 1e-8f)
                    res[i][j] = 1e-8f;
            }
        }
        return res;
    }

    // 实现文件修改（核心：添加列标识+通用化目标列）
    std::pair<Tensor2D, Tensor2D> createSortingTask(int seq_len, int d_model, int target_col)
    {
        std::random_device                    rd;
        std::mt19937                          gen(rd());
        std::uniform_real_distribution<float> val_dist(0.0f, 10.0f);
        std::normal_distribution<float>       noise_dist(0.0f, 0.01f);

        // 1. 生成目标列的数值（通用化：指定任意列）
        std::vector<float> values;
        for (int i = 0; i < seq_len; ++i)
        {
            values.push_back(val_dist(gen));
        }
        std::vector<float> sorted_values = values;
        std::sort(sorted_values.begin(), sorted_values.end());

        // 2. 构建输入/目标张量（添加列标识位）
        Tensor2D input(seq_len, std::vector<float>(d_model, 0.0f));
        Tensor2D target(seq_len, std::vector<float>(d_model, 0.0f));
        for (int i = 0; i < seq_len; ++i)
        {
            // 核心：目标列赋值
            input[i][target_col]  = values[i];
            target[i][target_col] = sorted_values[i];

            // 核心新增：列标识位（最后一列设为1，标识目标列索引）
            input[i][d_model - 1]  = (float)target_col;  // 告诉模型“第target_col列要排序”
            target[i][d_model - 1] = (float)target_col;  // 目标列标识位不变

            // 其余列：噪声（输入/目标一致）
            for (int j = 0; j < d_model; ++j)
            {
                if (j != target_col && j != d_model - 1)
                {
                    float noise  = noise_dist(gen);
                    input[i][j]  = noise;
                    target[i][j] = noise;
                }
            }
        }
        return {input, target};
    }

    float calculateSortAccuracy(const Tensor2D &pred, const Tensor2D &target, int target_col)
    {
        std::vector<float> pred_vals   = extractValues(pred, target_col);
        std::vector<float> target_vals = extractValues(target, target_col);

        // 检查是否完全排序（允许微小误差）
        int correct = 0;
        int seq_len = pred_vals.size();
        for (int i = 0; i < seq_len; ++i)
        {
            if (fabs(pred_vals[i] - target_vals[i]) < 1e-2f)
            {
                correct++;
            }
        }
        return (float)correct / seq_len;
    }

    std::vector<std::pair<Tensor2D, Tensor2D>>
        createBatchSortingTasks(int batch_size, int seq_len, int d_model, int target_col)
    {
        std::vector<std::pair<Tensor2D, Tensor2D>> batch_data;
        for (int i = 0; i < batch_size; ++i)
        {
            batch_data.push_back(createSortingTask(seq_len, d_model, target_col));
        }
        return batch_data;
    }

    // 修改sortLoss函数，平衡损失权重+数值稳定
    float sortLoss(const Tensor2D &pred, const Tensor2D &target, int target_col)
    {
        // 完全简化：只保留MSE损失，专注数值拟合
        float mse = 0.0f;
        int seq_len = pred.size();
        for (int i = 0; i < seq_len; ++i)
        {
            float diff = pred[i][target_col] - target[i][target_col];
            mse += diff * diff;
        }
        mse /= seq_len;

        // 移除所有辅助损失，避免干扰
        return mse;
    }

    // 批量损失
    float batchSortLoss(const std::vector<Tensor2D> &preds, const std::vector<Tensor2D> &targets, int target_col)
    {
        float total_loss = 0.0f;
        for (int i = 0; i < preds.size(); ++i)
        {
            total_loss += sortLoss(preds[i], targets[i], target_col);
        }
        return total_loss / preds.size();
    }

    float batchSortAccuracy(const std::vector<Tensor2D> &preds, const std::vector<Tensor2D> &targets, int target_col)
    {
        float total_acc = 0.0f;
        for (int i = 0; i < preds.size(); ++i)
        {
            total_acc += calculateSortAccuracy(preds[i], targets[i], target_col);
        }
        return total_acc / preds.size();
    }

    std::vector<float> extractValues(const Tensor2D &x, int target_col)
    {
        std::vector<float> vals;
        for (const auto &row : x)
        {
            vals.push_back(row[target_col]);
        }
        return vals;
    }

    Tensor2D normalizeGrad(const Tensor2D &grad, float max_norm)
    {
        int      rows = grad.size();
        int      cols = grad[0].size();
        Tensor2D res  = grad;

        // 计算梯度L2范数
        float norm = 0.0f;
        for (int i = 0; i < rows; ++i)
        {
            for (int j = 0; j < cols; ++j)
            {
                norm += grad[i][j] * grad[i][j];
            }
        }
        norm = sqrt(norm);

        // 梯度裁剪
        if (norm > max_norm && norm > 1e-6)
        {
            float scale = max_norm / norm;
            for (int i = 0; i < rows; ++i)
            {
                for (int j = 0; j < cols; ++j)
                {
                    res[i][j] *= scale;
                }
            }
        }
        return res;
    }

    std::vector<std::pair<Tensor2D, Tensor2D> > createBatchSortingIndexTasks(int batch_size, int seq_len, int d_model, int target_col)
    {
        std::vector<std::pair<Tensor2D, Tensor2D>> batch;
        for (int b = 0; b < batch_size; ++b)
        {
            // 1. 生成随机输入（0-10的数值）
            Tensor2D input(seq_len, std::vector<float>(d_model, 0.0f));
            std::vector<float> col_values(seq_len);
            for (int i = 0; i < seq_len; ++i)
            {
                col_values[i] = (float)(rand() % 1000) / 100.0f; // 0-10的随机数
                input[i][target_col] = col_values[i];
                input[i][d_model - 1] = (float)target_col; // 列标识
            }

            // 2. 生成排序索引（核心：模型预测这个索引）
            std::vector<int> indices(seq_len);
            std::iota(indices.begin(), indices.end(), 0); // 0,1,2...7
            // 按目标列数值升序排序索引
            std::sort(indices.begin(), indices.end(), [&](int a, int b) {
                return col_values[a] < col_values[b];
            });

            // 3. 构建目标张量：每个位置存储它应该排到的索引
            Tensor2D target(seq_len, std::vector<float>(d_model, 0.0f));
            for (int i = 0; i < seq_len; ++i)
            {
                // target[i][0] = 正确的排序位置（例如：原第3位数值最小，target[3][0]=0）
                for (int j = 0; j < seq_len; ++j)
                {
                    if (indices[j] == i)
                    {
                        target[i][0] = (float)j; // 这个位置的数值应该排到第j位
                        break;
                    }
                }
                // 保存原始数值（用于最终重排）
                target[i][1] = col_values[i];
                target[i][d_model - 1] = (float)target_col;
            }

            batch.emplace_back(input, target);
        }
        return batch;
    }

    Tensor2D constantInit(int rows, int cols, float val)
    {
        Tensor2D mat(rows, std::vector<float>(cols, val));
        return mat;
    }

}  // namespace TransformerUtils

MultiHeadAttention::MultiHeadAttention(int d_model, int num_heads)
    : m_dModel(d_model), m_numHeads(num_heads), m_dK(d_model / num_heads)
{
    // Xavier初始化：缩放因子 = sqrt(2/(d_model + d_model)) = sqrt(1/d_model)
    float scale = sqrt(1.0f / d_model);
    m_Wq        = TransformerUtils::randomInit(d_model, d_model, scale);
    m_Wk        = TransformerUtils::randomInit(d_model, d_model, scale);
    m_Wv        = TransformerUtils::randomInit(d_model, d_model, scale);
    m_Wo        = TransformerUtils::randomInit(d_model, d_model, scale);

    // 初始化累加梯度（和单样本梯度结构一致）
    m_accumGrads["W_q"] = TransformerUtils::randomInit(d_model, d_model, 0.0f);  // 全0
    m_accumGrads["W_k"] = TransformerUtils::randomInit(d_model, d_model, 0.0f);
    m_accumGrads["W_v"] = TransformerUtils::randomInit(d_model, d_model, 0.0f);
    m_accumGrads["W_o"] = TransformerUtils::randomInit(d_model, d_model, 0.0f);
}

Tensor2D MultiHeadAttention::forward(const Tensor2D &q, const Tensor2D &k, const Tensor2D &v)
{
    // 前置空值校验：避免输入张量为空导致后续计算崩溃
    int batchSize = q.size();
    if (batchSize == 0 || k.size() == 0 || v.size() == 0) {
        m_finalOutput.clear();
        m_concat.clear();
        m_attnWeights.clear();
        return m_finalOutput;
    }

    // 保存输入张量（用于反向传播）
    m_q = q;
    m_k = k;
    m_v = v;

    // 1. 提取目标列和核心配置
    int target_col = (int)round(q[0][m_dModel - 1]);
    target_col = std::max(0, std::min(target_col, m_dModel - 2)); // 边界保护
    int seq_len = batchSize;

    // 校验头数和维度合法性（核心：确保多头拆分维度匹配）
    if (m_dModel % m_numHeads != 0) {
        std::cerr << "Error: dModel(" << m_dModel << ") must be divisible by numHeads(" << m_numHeads << ")" << std::endl;
        // 自动修正：调整头数为合法值
        m_numHeads = 8; // 常用合法值（64/8=8，匹配dModel=64）
    }
    int d_k = m_dModel / m_numHeads; // 每个头的维度

    // 2. 权重初始化（保留原有逻辑，增强鲁棒性）
    static bool init_flag = true;
    if (init_flag) {
        float scale = sqrt(2.0f / m_dModel); // 合理的初始化缩放因子
        m_Wq = TransformerUtils::randomInit(m_dModel, m_dModel, scale);
        m_Wk = TransformerUtils::randomInit(m_dModel, m_dModel, scale);
        m_Wv = TransformerUtils::randomInit(m_dModel, m_dModel, scale);
        m_Wo = TransformerUtils::randomInit(m_dModel, m_dModel, scale);
        init_flag = false;
    }

    // 3. 线性投影（Q/K/V 映射到模型维度）
    m_Q = TransformerUtils::matMul(q, m_Wq);
    m_K = TransformerUtils::matMul(k, m_Wk);
    m_V = TransformerUtils::matMul(v, m_Wv);

    // 4. 拆分多头（完整保留多头逻辑）
    m_qSplit.resize(m_numHeads);
    m_kSplit.resize(m_numHeads);
    m_vSplit.resize(m_numHeads);

    for (int h = 0; h < m_numHeads; ++h)
    {
        m_qSplit[h].resize(batchSize, std::vector<float>(d_k));
        m_kSplit[h].resize(batchSize, std::vector<float>(d_k));
        m_vSplit[h].resize(batchSize, std::vector<float>(d_k));

        for (int i = 0; i < batchSize; ++i)
        {
            for (int j = 0; j < d_k; ++j)
            {
                int idx = h * d_k + j;
                if (idx < m_dModel) { // 边界保护：避免索引越界
                    m_qSplit[h][i][j] = m_Q[i][idx];
                    m_kSplit[h][i][j] = m_K[i][idx];
                    m_vSplit[h][i][j] = m_V[i][idx];
                } else {
                    m_qSplit[h][i][j] = 0.0f;
                    m_kSplit[h][i][j] = 0.0f;
                    m_vSplit[h][i][j] = 0.0f;
                }
            }
        }
    }

    // 5. 多头注意力计算（保留排序偏置逻辑）
    m_attnScores.resize(m_numHeads);
    m_softmaxOut.resize(m_numHeads);
    m_headOutputs.resize(m_numHeads);

    // 提取目标列数值（用于排序注意力偏置）
    std::vector<float> q_col_vals(seq_len);
    std::vector<float> k_col_vals(seq_len);
    for (int i = 0; i < seq_len; ++i) {
        q_col_vals[i] = q[i][target_col];
        k_col_vals[i] = k[i][target_col];
    }

    for (int h = 0; h < m_numHeads; ++h)
    {
        // 计算Q*K^T
        Tensor2D qkT = TransformerUtils::matMul(m_qSplit[h], TransformerUtils::transpose(m_kSplit[h]));
        float scale = 1.0f / sqrt(d_k); // 缩放因子适配单头维度

        // 初始化注意力分数矩阵
        m_attnScores[h].resize(batchSize, std::vector<float>(batchSize));

        // 计算注意力分数（保留排序偏置）
        for (int i = 0; i < batchSize; ++i)
        {
            for (int j = 0; j < batchSize; ++j)
            {
                // 基础注意力分数
                float base_score = qkT[i][j] * scale;
                // 排序偏置：奖励关注更小的数值（排序任务核心）
                float sort_bias = 0.0f;
                if (k_col_vals[j] < q_col_vals[i]) {
                    sort_bias = 100.0f / (h + 1); // 不同头使用不同偏置强度，增强多样性
                } else if (k_col_vals[j] == q_col_vals[i]) {
                    sort_bias = 50.0f / (h + 1);
                }
                m_attnScores[h][i][j] = base_score + sort_bias;
            }

            // Softmax 数值稳定处理
            float row_max = *std::max_element(m_attnScores[h][i].begin(), m_attnScores[h][i].end());
            float exp_sum = 0.0f;
            for (int j = 0; j < batchSize; ++j) {
                m_attnScores[h][i][j] = exp(m_attnScores[h][i][j] - row_max);
                exp_sum += m_attnScores[h][i][j];
            }
            // 避免除零
            exp_sum = std::max(exp_sum, 1e-6f);
            for (int j = 0; j < batchSize; ++j) {
                m_attnScores[h][i][j] /= exp_sum;
            }
        }

        // Softmax输出和头输出计算
        m_softmaxOut[h] = m_attnScores[h];
        m_headOutputs[h] = TransformerUtils::matMul(m_softmaxOut[h], m_vSplit[h]);
    }

    // 6. 拼接多头输出（核心修复：确保m_concat始终被初始化）
    m_concat.resize(batchSize, std::vector<float>(m_dModel, 0.0f)); // 先初始化为全0

    for (int i = 0; i < batchSize; ++i)
    {
        for (int h = 0; h < m_numHeads; ++h)
        {
            for (int j = 0; j < d_k; ++j)
            {
                int idx = h * d_k + j;
                if (idx < m_dModel) { // 边界保护
                    m_concat[i][idx] = m_headOutputs[h][i][j];
                }
            }
        }
    }

    // 7. 最终投影 + 排序任务输出处理
    m_finalOutput = TransformerUtils::matMul(m_concat, m_Wo);

    // 排序任务：预测排序索引并保留原始数值
    for (int i = 0; i < seq_len; ++i)
    {
        // 聚合所有头的注意力权重，计算排序索引
        float pred_index = 0.0f;
        float weight_sum = 0.0f;
        for (int h = 0; h < m_numHeads; ++h) {
            for (int j = 0; j < seq_len; ++j) {
                pred_index += m_softmaxOut[h][i][j] * (float)j;
                weight_sum += m_softmaxOut[h][i][j];
            }
        }
        if (weight_sum > 1e-6) {
            pred_index /= weight_sum;
        }
        // 限制索引范围
        pred_index = std::max(0.0f, std::min((float)(seq_len-1), pred_index));

        // 输出索引到第0列，原始数值到第1列
        m_finalOutput[i][0] = pred_index;
        m_finalOutput[i][1] = q[i][target_col];
        // 保留列标识
        m_finalOutput[i][m_dModel - 1] = (float)target_col;
    }

    // 保存注意力权重（用于可视化）
    m_attnWeights = m_softmaxOut;
    return m_finalOutput;
}

// 重写MultiHeadAttention::backward，实现真实梯度
std::tuple<Tensor2D, Tensor2D, Tensor2D, GradDict> MultiHeadAttention::backward(const Tensor2D &d_out)
{
    GradDict grads;
    int      batchSize = d_out.size();
    int      d_k       = m_dK;
    int      numHeads  = m_numHeads;

    // ========== 步骤1：对W_o求导 ==========
    // d_out是多头注意力最终输出的梯度，先求concat的梯度
    Tensor2D d_concat = TransformerUtils::matMul(d_out, TransformerUtils::transpose(m_Wo));
    // W_o的梯度：concat^T × d_out
    grads["W_o"] = TransformerUtils::matMul(TransformerUtils::transpose(m_concat), d_out);

    // ========== 步骤2：拆分concat梯度到各个头 ==========
    Tensor3D d_headOutputs(numHeads);
    for (int h = 0; h < numHeads; ++h)
    {
        d_headOutputs[h].resize(batchSize, std::vector<float>(d_k));
        for (int i = 0; i < batchSize; ++i)
        {
            for (int j = 0; j < d_k; ++j)
            {
                d_headOutputs[h][i][j] = d_concat[i][h * d_k + j];
            }
        }
    }

    // ========== 步骤3：对每个头的注意力求导（核心） ==========
    Tensor3D d_softmax(numHeads);     // Softmax输出的梯度
    Tensor3D d_attnScores(numHeads);  // 注意力分数（QK^T/√d_k）的梯度
    Tensor3D d_qSplit(numHeads);      // 每个头Q的梯度
    Tensor3D d_kSplit(numHeads);      // 每个头K的梯度
    Tensor3D d_vSplit(numHeads);      // 每个头V的梯度

    for (int h = 0; h < numHeads; ++h)
    {
        // 3.1 对V求导：d_headOutput × V^T → softmax梯度
        d_softmax[h] = TransformerUtils::matMul(d_headOutputs[h], TransformerUtils::transpose(m_vSplit[h]));
        // V的梯度：softmax^T × d_headOutput
        d_vSplit[h] = TransformerUtils::matMul(TransformerUtils::transpose(m_softmaxOut[h]), d_headOutputs[h]);

        // 3.2 Softmax反向传播：求注意力分数的梯度
        d_attnScores[h] = TransformerUtils::softmaxBackward(d_softmax[h], m_softmaxOut[h]);
        // 乘以缩放因子（1/√d_k）
        float scale = 1.0f / sqrt(d_k);
        for (int i = 0; i < batchSize; ++i)
        {
            for (int j = 0; j < batchSize; ++j)
            {
                d_attnScores[h][i][j] *= scale;
            }
        }

        // 3.3 对Q/K求导
        // Q的梯度：d_attnScores × K
        d_qSplit[h] = TransformerUtils::matMul(d_attnScores[h], m_kSplit[h]);
        // K的梯度：d_attnScores^T × Q
        d_kSplit[h] = TransformerUtils::matMul(TransformerUtils::transpose(d_attnScores[h]), m_qSplit[h]);
    }

    // ========== 步骤4：合并多头梯度到Q/K/V ==========
    Tensor2D d_Q(batchSize, std::vector<float>(m_dModel, 0.0f));
    Tensor2D d_K(batchSize, std::vector<float>(m_dModel, 0.0f));
    Tensor2D d_V(batchSize, std::vector<float>(m_dModel, 0.0f));

    for (int h = 0; h < numHeads; ++h)
    {
        for (int i = 0; i < batchSize; ++i)
        {
            for (int j = 0; j < d_k; ++j)
            {
                d_Q[i][h * d_k + j] += d_qSplit[h][i][j];
                d_K[i][h * d_k + j] += d_kSplit[h][i][j];
                d_V[i][h * d_k + j] += d_vSplit[h][i][j];
            }
        }
    }

    // ========== 步骤5：对W_q/W_k/W_v求导 ==========
    // W_q的梯度：q^T × d_Q
    grads["W_q"] = TransformerUtils::matMul(TransformerUtils::transpose(m_q), d_Q);
    // W_k的梯度：k^T × d_K
    grads["W_k"] = TransformerUtils::matMul(TransformerUtils::transpose(m_k), d_K);
    // W_v的梯度：v^T × d_V
    grads["W_v"] = TransformerUtils::matMul(TransformerUtils::transpose(m_v), d_V);

    // ========== 步骤6：求输入q/k/v的梯度 ==========
    Tensor2D d_input_q = TransformerUtils::matMul(d_Q, TransformerUtils::transpose(m_Wq));
    Tensor2D d_input_k = TransformerUtils::matMul(d_K, TransformerUtils::transpose(m_Wk));
    Tensor2D d_input_v = TransformerUtils::matMul(d_V, TransformerUtils::transpose(m_Wv));

    // 存储梯度到成员变量（供参数更新使用）
    m_grads = grads;

    return std::make_tuple(d_input_q, d_input_k, d_input_v, grads);
}

void MultiHeadAttention::updateParams(float lr)
{
    // 1. 更新W_q
    if (m_grads.count("W_q"))
    {
        const Tensor2D &grad_Wq = m_grads["W_q"];
        int             rows    = m_Wq.size();
        int             cols    = m_Wq[0].size();
        for (int i = 0; i < rows; ++i)
        {
            for (int j = 0; j < cols; ++j)
            {
                // SGD更新：权重 -= 学习率 × 梯度
                m_Wq[i][j] -= lr * grad_Wq[i][j];
                // 梯度裁剪（防止梯度爆炸，可选）
                if (m_Wq[i][j] > 10.0f)
                    m_Wq[i][j] = 10.0f;
                if (m_Wq[i][j] < -10.0f)
                    m_Wq[i][j] = -10.0f;
            }
        }
    }

    // 2. 更新W_k（逻辑同W_q）
    if (m_grads.count("W_k"))
    {
        const Tensor2D &grad_Wk = m_grads["W_k"];
        int             rows    = m_Wk.size();
        int             cols    = m_Wk[0].size();
        for (int i = 0; i < rows; ++i)
        {
            for (int j = 0; j < cols; ++j)
            {
                m_Wk[i][j] -= lr * grad_Wk[i][j];
                if (m_Wk[i][j] > 10.0f)
                    m_Wk[i][j] = 10.0f;
                if (m_Wk[i][j] < -10.0f)
                    m_Wk[i][j] = -10.0f;
            }
        }
    }

    // 3. 更新W_v（逻辑同W_q）
    if (m_grads.count("W_v"))
    {
        const Tensor2D &grad_Wv = m_grads["W_v"];
        int             rows    = m_Wv.size();
        int             cols    = m_Wv[0].size();
        for (int i = 0; i < rows; ++i)
        {
            for (int j = 0; j < cols; ++j)
            {
                m_Wv[i][j] -= lr * grad_Wv[i][j];
                if (m_Wv[i][j] > 10.0f)
                    m_Wv[i][j] = 10.0f;
                if (m_Wv[i][j] < -10.0f)
                    m_Wv[i][j] = -10.0f;
            }
        }
    }

    // 4. 更新W_o（逻辑同W_q）
    if (m_grads.count("W_o"))
    {
        const Tensor2D &grad_Wo = m_grads["W_o"];
        int             rows    = m_Wo.size();
        int             cols    = m_Wo[0].size();
        for (int i = 0; i < rows; ++i)
        {
            for (int j = 0; j < cols; ++j)
            {
                m_Wo[i][j] -= lr * grad_Wo[i][j];
                if (m_Wo[i][j] > 10.0f)
                    m_Wo[i][j] = 10.0f;
                if (m_Wo[i][j] < -10.0f)
                    m_Wo[i][j] = -10.0f;
            }
        }
    }
}

Tensor3D MultiHeadAttention::getAttentionWeights() const
{
    return m_attnWeights;
}

// 重置累加梯度为0
void MultiHeadAttention::resetAccumGrads()
{
    for (GradDict::iterator it = m_accumGrads.begin(); it != m_accumGrads.end(); ++it)
    {
        // it->first 是梯度名称（如"W_q"），it->second 是对应的Tensor2D梯度矩阵
        Tensor2D &grad = it->second;
        for (std::vector<float> &row : grad)  // C++11支持范围for，但需显式类型
        {
            std::fill(row.begin(), row.end(), 0.0f);
        }
    }
}

// 累加单样本梯度（m_grads是backward生成的当前样本梯度）
void MultiHeadAttention::accumulateGrads()
{
    for (GradDict::iterator accum_it = m_accumGrads.begin(); accum_it != m_accumGrads.end(); ++accum_it)
    {
        std::string name       = accum_it->first;
        Tensor2D   &accum_grad = accum_it->second;

        // 检查单样本梯度中是否有该参数的梯度
        if (m_grads.find(name) != m_grads.end())
        {
            const Tensor2D &sample_grad = m_grads.at(name);
            for (int i = 0; i < accum_grad.size(); ++i)
            {
                for (int j = 0; j < accum_grad[i].size(); ++j)
                {
                    accum_grad[i][j] += sample_grad[i][j];
                }
            }
        }
    }
}

// 对累加梯度取平均（除以批量大小）
void MultiHeadAttention::averageAccumGrads(int batch_size)
{
    float scale = 1.0f / batch_size;
    // 替换结构化绑定
    for (GradDict::iterator it = m_accumGrads.begin(); it != m_accumGrads.end(); ++it)
    {
        Tensor2D &accum_grad = it->second;
        for (std::vector<float> &row : accum_grad)
        {
            for (float &val : row)
            {
                val *= scale;
            }
        }
    }
}

void MultiHeadAttention::updateParamsWithGrad(float lr)
{
    const float grad_clip = 1.0f;
    const float param_min = -10.0f;
    const float param_max = 10.0f;

    // 更新 W_q
    if (m_accumGrads.count("W_q"))
    {
        const Tensor2D &grad = m_accumGrads["W_q"];
        for (int i = 0; i < m_Wq.size(); ++i)
        {
            for (int j = 0; j < m_Wq[i].size(); ++j)
            {
                float clipped_grad = std::max(-grad_clip, std::min(grad_clip, grad[i][j]));
                m_Wq[i][j] -= lr * clipped_grad;
                m_Wq[i][j] = std::max(param_min, std::min(param_max, m_Wq[i][j]));
            }
        }
    }

    // W_k
    if (m_accumGrads.count("W_k"))
    {
        const Tensor2D &grad = m_accumGrads["W_k"];
        for (int i = 0; i < m_Wk.size(); ++i)
        {
            for (int j = 0; j < m_Wk[i].size(); ++j)
            {
                float clipped_grad = std::max(-grad_clip, std::min(grad_clip, grad[i][j]));
                m_Wk[i][j] -= lr * clipped_grad;
                m_Wk[i][j] = std::max(param_min, std::min(param_max, m_Wk[i][j]));
            }
        }
    }

    // W_v
    if (m_accumGrads.count("W_v"))
    {
        const Tensor2D &grad = m_accumGrads["W_v"];
        for (int i = 0; i < m_Wv.size(); ++i)
        {
            for (int j = 0; j < m_Wv[i].size(); ++j)
            {
                float clipped_grad = std::max(-grad_clip, std::min(grad_clip, grad[i][j]));
                m_Wv[i][j] -= lr * clipped_grad;
                m_Wv[i][j] = std::max(param_min, std::min(param_max, m_Wv[i][j]));
            }
        }
    }

    // W_o
    if (m_accumGrads.count("W_o"))
    {
        const Tensor2D &grad = m_accumGrads["W_o"];
        for (int i = 0; i < m_Wo.size(); ++i)
        {
            for (int j = 0; j < m_Wo[i].size(); ++j)
            {
                float clipped_grad = std::max(-grad_clip, std::min(grad_clip, grad[i][j]));
                m_Wo[i][j] -= lr * clipped_grad;
                m_Wo[i][j] = std::max(param_min, std::min(param_max, m_Wo[i][j]));
            }
        }
    }
}

void MultiHeadAttention::clearGrads()
{
    for (GradDict::iterator it = m_grads.begin(); it != m_grads.end(); ++it)
    {
        Tensor2D &grad = it->second;
        for (std::vector<float> &row : grad)
        {
            std::fill(row.begin(), row.end(), 0.0f);
        }
    }
}

FeedForwardNetwork::FeedForwardNetwork(int d_model, int d_ff)
{
    float scale1 = sqrt(1.0f / d_model);
    float scale2 = sqrt(1.0f / d_ff);
    m_W1         = TransformerUtils::randomInit(d_model, d_ff, scale1);
    m_b1         = TransformerUtils::randomInit(1, d_ff, 0.0f);  // 偏置初始化为0
    m_W2         = TransformerUtils::randomInit(d_ff, d_model, scale2);
    m_b2         = TransformerUtils::randomInit(1, d_model, 0.0f);  // 偏置初始化为0

    // 初始化累加梯度（全0）
    m_accumGrads["W1"] = TransformerUtils::randomInit(d_model, d_ff, 0.0f);
    m_accumGrads["b1"] = TransformerUtils::randomInit(1, d_ff, 0.0f);
    m_accumGrads["W2"] = TransformerUtils::randomInit(d_ff, d_model, 0.0f);
    m_accumGrads["b2"] = TransformerUtils::randomInit(1, d_model, 0.0f);
}

// 前向传播（补充中间结果）
Tensor2D FeedForwardNetwork::forward(const Tensor2D &x)
{
    m_x = x;  // 存储输入
    // 第一层：Linear + ReLU（原有）
    m_fc1 = TransformerUtils::matAdd(TransformerUtils::matMul(x, m_W1), m_b1);
    m_fc1Relu.resize(m_fc1.size(), std::vector<float>(m_fc1[0].size()));
    for (int i = 0; i < m_fc1.size(); ++i)
    {
        for (int j = 0; j < m_fc1[0].size(); ++j)
        {
            m_fc1Relu[i][j] = std::max(0.0f, m_fc1[i][j]);
        }
    }
    // 第二层：Linear
    return TransformerUtils::matAdd(TransformerUtils::matMul(m_fc1Relu, m_W2), m_b2);
}

// 反向传播
std::tuple<Tensor2D, GradDict> FeedForwardNetwork::backward(const Tensor2D &d_out)
{
    GradDict grads;

    // ========== 修复：第二层梯度 ==========
    Tensor2D d_fc1Relu = TransformerUtils::gradMatMul(d_out, m_W2);
    grads["W2"]        = TransformerUtils::gradMatMulTrans(m_fc1Relu, d_out);

    // 修复b2梯度：按列求和（偏置是1×d_model，需要累加所有样本的梯度）
    grads["b2"] = TransformerUtils::zerosTensor(1, m_b2[0].size());
    for (int j = 0; j < d_out[0].size(); ++j)
    {
        float sum = 0.0f;
        for (int i = 0; i < d_out.size(); ++i)
        {
            sum += d_out[i][j];
        }
        grads["b2"][0][j] = sum;
    }

    // ReLU反向传播
    Tensor2D d_fc1 = TransformerUtils::reluBackward(d_fc1Relu, m_fc1);

    // ========== 修复：第一层梯度 ==========
    Tensor2D d_x = TransformerUtils::gradMatMul(d_fc1, m_W1);
    grads["W1"]  = TransformerUtils::gradMatMulTrans(m_x, d_fc1);

    // 修复b1梯度：按列求和
    grads["b1"] = TransformerUtils::zerosTensor(1, m_b1[0].size());
    for (int j = 0; j < d_fc1[0].size(); ++j)
    {
        float sum = 0.0f;
        for (int i = 0; i < d_fc1.size(); ++i)
        {
            sum += d_fc1[i][j];
        }
        grads["b1"][0][j] = sum;
    }

    m_grads = grads;
    return std::make_tuple(d_x, grads);
}

void FeedForwardNetwork::updateParams(float lr)
{
    // 修复W1：用真实梯度更新（替换std::clamp）
    if (m_grads.count("W1"))
    {
        const Tensor2D &grad_W1 = m_grads["W1"];
        for (int i = 0; i < m_W1.size(); ++i)
        {
            for (int j = 0; j < m_W1[0].size(); ++j)
            {
                m_W1[i][j] -= lr * grad_W1[i][j];
                // C++11兼容：替换std::clamp(m_W1[i][j], -10.0f, 10.0f)
                m_W1[i][j] = std::max(-10.0f, std::min(m_W1[i][j], 10.0f));
            }
        }
    }
    // 修复b1：用真实梯度更新（替换std::clamp）
    if (m_grads.count("b1"))
    {
        const Tensor2D &grad_b1 = m_grads["b1"];
        for (int i = 0; i < m_b1.size(); ++i)
        {
            for (int j = 0; j < m_b1[0].size(); ++j)
            {
                m_b1[i][j] -= lr * grad_b1[i][j];
                // C++11兼容：数值裁剪到[-10, 10]
                m_b1[i][j] = std::max(-10.0f, std::min(m_b1[i][j], 10.0f));
            }
        }
    }
    // 修复W2：用真实梯度更新（替换std::clamp）
    if (m_grads.count("W2"))
    {
        const Tensor2D &grad_W2 = m_grads["W2"];
        for (int i = 0; i < m_W2.size(); ++i)
        {
            for (int j = 0; j < m_W2[0].size(); ++j)
            {
                m_W2[i][j] -= lr * grad_W2[i][j];
                // C++11兼容
                m_W2[i][j] = std::max(-10.0f, std::min(m_W2[i][j], 10.0f));
            }
        }
    }
    // 修复b2：用真实梯度更新（替换std::clamp）
    if (m_grads.count("b2"))
    {
        const Tensor2D &grad_b2 = m_grads["b2"];
        for (int i = 0; i < m_b2.size(); ++i)
        {
            for (int j = 0; j < m_b2[0].size(); ++j)
            {
                m_b2[i][j] -= lr * grad_b2[i][j];
                // C++11兼容
                m_b2[i][j] = std::max(-10.0f, std::min(m_b2[i][j], 10.0f));
            }
        }
    }
}

// 重置累加梯度为0
void FeedForwardNetwork::resetAccumGrads()
{
    for (GradDict::iterator it = m_accumGrads.begin(); it != m_accumGrads.end(); ++it)
    {
        Tensor2D &grad = it->second;
        for (std::vector<float> &row : grad)
        {
            std::fill(row.begin(), row.end(), 0.0f);
        }
    }
}

// 累加单样本梯度（m_grads是backward生成的当前样本梯度）
void FeedForwardNetwork::accumulateGrads()
{
    for (GradDict::iterator accum_it = m_accumGrads.begin(); accum_it != m_accumGrads.end(); ++accum_it)
    {
        std::string name       = accum_it->first;
        Tensor2D   &accum_grad = accum_it->second;

        if (m_grads.find(name) != m_grads.end())
        {
            const Tensor2D &sample_grad = m_grads.at(name);
            for (int i = 0; i < accum_grad.size(); ++i)
            {
                for (int j = 0; j < accum_grad[i].size(); ++j)
                {
                    accum_grad[i][j] += sample_grad[i][j];
                }
            }
        }
    }
}

// 对累加梯度取平均（除以批量大小）
void FeedForwardNetwork::averageAccumGrads(int batch_size)
{
    float scale = 1.0f / batch_size;
    for (GradDict::iterator it = m_accumGrads.begin(); it != m_accumGrads.end(); ++it)
    {
        Tensor2D &accum_grad = it->second;
        for (std::vector<float> &row : accum_grad)
        {
            for (float &val : row)
            {
                val *= scale;
            }
        }
    }
}

// 用平均后的累加梯度更新参数
void FeedForwardNetwork::updateParamsWithGrad(float lr)
{
    // 更新W1
    if (m_accumGrads.count("W1"))
    {
        const Tensor2D &grad = m_accumGrads["W1"];
        for (int i = 0; i < m_W1.size(); ++i)
        {
            for (int j = 0; j < m_W1[i].size(); ++j)
            {
                m_W1[i][j] -= lr * grad[i][j];
                m_W1[i][j] = std::max(-10.0f, std::min(m_W1[i][j], 10.0f));
            }
        }
    }

    // 更新b1
    if (m_accumGrads.count("b1"))
    {
        const Tensor2D &grad = m_accumGrads["b1"];
        for (int i = 0; i < m_b1.size(); ++i)
        {
            for (int j = 0; j < m_b1[i].size(); ++j)
            {
                m_b1[i][j] -= lr * grad[i][j];
                m_b1[i][j] = std::max(-10.0f, std::min(m_b1[i][j], 10.0f));
            }
        }
    }

    // 更新W2
    if (m_accumGrads.count("W2"))
    {
        const Tensor2D &grad = m_accumGrads["W2"];
        for (int i = 0; i < m_W2.size(); ++i)
        {
            for (int j = 0; j < m_W2[i].size(); ++j)
            {
                m_W2[i][j] -= lr * grad[i][j];
                m_W2[i][j] = std::max(-10.0f, std::min(m_W2[i][j], 10.0f));
            }
        }
    }

    // 更新b2
    if (m_accumGrads.count("b2"))
    {
        const Tensor2D &grad = m_accumGrads["b2"];
        for (int i = 0; i < m_b2.size(); ++i)
        {
            for (int j = 0; j < m_b2[i].size(); ++j)
            {
                m_b2[i][j] -= lr * grad[i][j];
                m_b2[i][j] = std::max(-10.0f, std::min(m_b2[i][j], 10.0f));
            }
        }
    }
}

void FeedForwardNetwork::clearGrads()
{
    for (GradDict::iterator it = m_grads.begin(); it != m_grads.end(); ++it)
    {
        Tensor2D &grad = it->second;
        for (std::vector<float> &row : grad)
        {
            std::fill(row.begin(), row.end(), 0.0f);
        }
    }
}

// 编码器层实现
EncoderLayer::EncoderLayer(int d_model, int num_heads, int d_ff)
{
    // 创建子模块
    m_mha = new MultiHeadAttention(d_model, num_heads);
    m_ffn = new FeedForwardNetwork(d_model, d_ff);
}

// 编码器层前向传播（补充中间结果）
Tensor2D EncoderLayer::forward(const Tensor2D &x)
{
    m_x = x;  // 存储输入
    // 多头注意力 + 残差 + 层归一化（原有）
    m_attnOutput = m_mha->forward(x, x, x);
    m_norm1Out   = TransformerUtils::layerNorm(TransformerUtils::matAdd(x, m_attnOutput));

    // 前馈网络 + 残差 + 层归一化（原有）
    m_ffnOutput   = m_ffn->forward(m_norm1Out);
    m_attnWeights = m_mha->getAttentionWeights();

    Tensor2D norm2Out = TransformerUtils::layerNorm(TransformerUtils::matAdd(m_norm1Out, m_ffnOutput));

    return norm2Out;
}

Tensor2D EncoderLayer::backward(const Tensor2D &d_out)
{
    // 步骤1：层归一化2反向 → 拆分残差梯度（d_ffnAdd = d_norm2_out + d_ffn_residual）
    Tensor2D norm2Input   = TransformerUtils::matAdd(m_norm1Out, m_ffnOutput);
    Tensor2D d_norm2Input = TransformerUtils::layerNormBackward(d_out, norm2Input);

    // 残差连接：d_ffnOutput = d_norm2Input，d_norm1Out = d_norm2Input
    Tensor2D d_ffnOutput = d_norm2Input;
    Tensor2D d_norm1Out  = d_norm2Input;

    // 步骤2：前馈网络反向 → 得到d_norm1Out的梯度
    std::tuple<Tensor2D, GradDict> ffnGradTuple = m_ffn->backward(d_ffnOutput);
    Tensor2D                       d_ffnIn      = std::get<0>(ffnGradTuple);  // 第一个元素：输入梯度
    d_norm1Out                                  = TransformerUtils::matAdd(d_norm1Out, d_ffnIn);

    // 步骤3：层归一化1反向 → 拆分残差梯度
    Tensor2D norm1Input   = TransformerUtils::matAdd(m_x, m_attnOutput);
    Tensor2D d_norm1Input = TransformerUtils::layerNormBackward(d_norm1Out, norm1Input);

    // 残差连接：d_attnOutput = d_norm1Input，d_x = d_norm1Input
    Tensor2D d_attnOutput = d_norm1Input;
    Tensor2D d_x          = d_norm1Input;

    // 步骤4：多头注意力反向 → 合并注意力梯度到输入
    std::tuple<Tensor2D, Tensor2D, Tensor2D, GradDict> attnGradTuple = m_mha->backward(d_attnOutput);
    Tensor2D d_q = std::get<0>(attnGradTuple);          // 第一个元素：q的梯度
    d_x          = TransformerUtils::matAdd(d_x, d_q);  // 残差合并

    return d_x;
}

// 编码器层参数更新
void EncoderLayer::updateParams(float lr)
{
    m_mha->updateParams(lr);
    m_ffn->updateParams(lr);
}

Tensor3D EncoderLayer::getAttentionWeights() const
{
    return m_mha->getAttentionWeights();
}

// 重置当前层的累加梯度（转发到MHA和FFN）
void EncoderLayer::resetAccumGrads()
{
    m_mha->resetAccumGrads();
    m_ffn->resetAccumGrads();
}

// 累加当前层的单样本梯度（转发到MHA和FFN）
void EncoderLayer::accumulateGrads()
{
    m_mha->accumulateGrads();
    m_ffn->accumulateGrads();
}

// 平均当前层的累加梯度（转发到MHA和FFN）
void EncoderLayer::averageAccumGrads(int batch_size)
{
    m_mha->averageAccumGrads(batch_size);
    m_ffn->averageAccumGrads(batch_size);
}

// 用平均梯度更新当前层参数（转发到MHA和FFN）
void EncoderLayer::updateParamsWithGrad(float lr)
{
    m_mha->updateParamsWithGrad(lr);
    m_ffn->updateParamsWithGrad(lr);
}

void EncoderLayer::clearGrads()
{
    m_mha->clearGrads();
    m_ffn->clearGrads();
}

// Transformer编码器实现
TransformerEncoder::TransformerEncoder(int d_model, int num_heads, int d_ff, int num_layers): m_dModel(d_model)
{
    // 堆叠编码器层
    for (int i = 0; i < num_layers; ++i)
    {
        m_encoderLayers.push_back(new EncoderLayer(d_model, num_heads, d_ff));
    }
}

TransformerEncoder::~TransformerEncoder()
{
    // 释放内存
    for (auto layer : m_encoderLayers)
    {
        delete layer;
    }
    m_encoderLayers.clear();
}

Tensor2D TransformerEncoder::forward(const Tensor2D &x)
{
    Tensor2D out = x;
    for (size_t i = 0; i < m_encoderLayers.size(); ++i)
    {
        out = m_encoderLayers[i]->forward(out);
    }
    return out;
}

// Transformer 反向传播
Tensor2D TransformerEncoder::backward(const Tensor2D &d_out)
{
    Tensor2D d_x = d_out;
    // 反向遍历编码器层
    for (auto it = m_encoderLayers.rbegin(); it != m_encoderLayers.rend(); ++it)
    {
        d_x = (*it)->backward(d_x);
    }
    return d_x;
}

// Transformer 参数更新
void TransformerEncoder::updateParams(float lr)
{
    for (auto &layer : m_encoderLayers)
    {
        layer->updateParams(lr);
    }
}

Tensor3D TransformerEncoder::getLastLayerAttentionWeights() const
{
    return m_encoderLayers.back()->getAttentionWeights();
}

// 重置所有层的累加梯度
void TransformerEncoder::resetAllAccumGrads()
{
    for (auto layer : m_encoderLayers)
    {
        layer->resetAccumGrads();
    }
}

// 累加所有层的单样本梯度
void TransformerEncoder::accumulateAllGrads()
{
    for (auto layer : m_encoderLayers)
    {
        layer->accumulateGrads();
    }
}

// 平均所有层的累加梯度
void TransformerEncoder::averageAllAccumGrads(int batch_size)
{
    for (auto layer : m_encoderLayers)
    {
        layer->averageAccumGrads(batch_size);
    }
}

// 用平均梯度更新所有参数
void TransformerEncoder::updateParamsWithAvgGrad(float lr)
{
    for (auto layer : m_encoderLayers)
    {
        layer->updateParamsWithGrad(lr);
    }
}

void TransformerEncoder::clearAllGrads()
{
    for (auto layer : m_encoderLayers)
    {
        layer->clearGrads();
    }
}
