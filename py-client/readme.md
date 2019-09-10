### Requirements:
```bash
pip install SimpleITK numpy 
```

### Sample
```python
import client_api
client = client_api.AIAAClient(server_ip='0.0.0.0', server_port=5000)

# List Models
models = client.model_list(label='spleen')

# Run Segmentation
client.segmentation(
  model='segmentation_ct_spleen', 
  image_in='input3D.nii.gz',
  image_out='seg_mask3D.nii.gz')

# Run Annotation
client.dextr3d(
   model='annotation_ct_spleen',
   point_set=[[52,181,126],[137,152,171],[97,113,148],[78,227,115],[72,178,80],[97,175,188]],
   image_in='input3D.nii.gz',
   image_out='ann_mask3D.nii.gz')
```

#### Running Tests
```bash
python test_aiaa_server.py --test_config aas_tests.json \
       --server_ip 0.0.0.0 --server_port 5000
```

test_aiaa_server.py gives method to test all the possible APIs supported under by NVIDIA AI-Assisted Annotation and required configurations are specified by aas_tests.json:
