# D3D12DescriptorCopyBenchtest
This project intend to create a benchmark for comparing the overhead of different ways to get per-frame D3D12 descriptors to the GPU.

## Test scenario
An application has N textures that will be bound as _shader resouce veiws_ (SRV) every frame. Each texture require a corresponding
descriptor in a _shader visible descriptor heap_ (SVDH) in order to be bound. The application will write all descriptors to the SVDH
every frame.

This project will evaluate the overhead of varius approaches for descriptors to end up in the SVDH. Each approach will be considered
as a _test_ and must follow some rules:
- The will only exist one SVDH.
- The SVDH will be considered empty at the start of a test.
- The test is finished when all N descriptors exist in the SVDH.

Tests to evaluate:
- Create descriptors individually in the SVDH.
- Create descriptors in separate non-shader visible descriptor heaps. Copy each descriptor individually to the SVDH. 
- Create descriptors in a common non-shader visible descriptor heap. Copy each descriptor individually to the SVDH. 
- Create descriptors in a common non-shader visible descriptor heap. Copy all descriptors together to the SVDH.
