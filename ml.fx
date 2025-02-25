import "tensor.fx";
import "math.fx";

namespace ml 
{
    class CodeTranslationNetwork 
    {
        struct NetworkConfig 
        {
            int{32}[] layerSizes;
            float{64} learningRate;
        };

        struct Layer 
        {
            Tensor.TensorState weights;
            Tensor.TensorState biases;
        };

        object NeuralNet 
        {
            Layer[] init(NetworkConfig config) 
            {
                Layer[] layers = memalloc(Layer[length(config.layerSizes) - 1]);
                
                for (int{32} i = 0; i < length(config.layerSizes) - 1; i++) 
                {
                    int{32}[] weightShape = [config.layerSizes[i], config.layerSizes[i+1]];
                    
                    layers[i].weights = Tensor.TensorOps.random(weightShape, -1.0, 1.0);
                    
                    int{32}[] biasShape = [config.layerSizes[i+1]];
                    layers[i].biases = Tensor.TensorOps.zeros(biasShape);
                };
                
                return layers;
            };

            float{64} relu(float{64} x) 
            {
                return x > 0.0 ? x : 0.0;
            };

            Tensor.TensorState forward(Layer[] layers, Tensor.TensorState input) 
            {
                Tensor.TensorState current = input;
                
                for (int{32} i = 0; i < length(layers); i++) 
                {
                    Tensor.TensorState z = Tensor.TensorOps.matmul(current, layers[i].weights);
                    
                    for (int{32} j = 0; j < z.shape[1]; j++) 
                    {
                        int{32}[] indices = [0, j];
                        float{64} val = Tensor.TensorOps.get(z, indices);
                        Tensor.TensorOps.set(z, indices, val + Tensor.TensorOps.get(layers[i].biases, [j]));
                    };
                    
                    current = applyRelu(z);
                };
                
                return current;
            };

            Tensor.TensorState applyRelu(Tensor.TensorState tensor) 
            {
                Tensor.TensorState result = Tensor.TensorOps.copy(tensor);
                
                int{32} totalElements = 1;
                for (int{32} i = 0; i < tensor.rank; i++) 
                {
                    totalElements = totalElements * tensor.shape[i];
                };
                
                for (int{32} i = 0; i < totalElements; i++) 
                {
                    float{64}[] indices = [i];
                    float{64} val = Tensor.TensorOps.get(tensor, indices);
                    Tensor.TensorOps.set(result, indices, relu(val));
                };
                
                return result;
            };

            float{64} train(Layer[] layers, 
                            Tensor.TensorState input, 
                            Tensor.TensorState expectedOutput) 
            {
                Tensor.TensorState prediction = forward(layers, input);
                
                float{64} loss = 0.0;
                for (int{32} i = 0; i < expectedOutput.shape[1]; i++) 
                {
                    float{64} diff = Tensor.TensorOps.get(prediction, [0, i]) - 
                                     Tensor.TensorOps.get(expectedOutput, [0, i]);
                    loss = loss + diff * diff;
                };
                
                return loss / float{64}:expectedOutput.shape[1];
            };
        };
    };
};
