import React, { useState } from 'react';
import { Container, Row, Col, Card, Button, Form, Spinner, Alert } from 'react-bootstrap';
import './App.css';

function App() {
  const [selectedFile, setSelectedFile] = useState(null);
  const [imagePreview, setImagePreview] = useState(null);
  const [outputImage, setOutputImage] = useState(null);
  const [detectionResults, setDetectionResults] = useState(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState(null);

  const handleFileChange = (event) => {
    const file = event.target.files[0];
    if (file) {
      setSelectedFile(file);
      const reader = new FileReader();
      reader.onloadend = () => {
        setImagePreview(reader.result);
      };
      reader.readAsDataURL(file);
      setError(null); // Clear previous errors
      setOutputImage(null); // Clear previous results
      setDetectionResults(null);
    } else {
      setSelectedFile(null);
      setImagePreview(null);
    }
  };

  const handleDetect = async () => {
    if (!selectedFile) {
      setError("请选择一张图片进行检测。");
      return;
    }

    setLoading(true);
    setError(null);
    setOutputImage(null);
    setDetectionResults(null);

    try {
      const reader = new FileReader();
      reader.onloadend = async () => {
        const base64Image = reader.result.split(',')[1]; // Get base64 string without data:image/jpeg;base64, prefix

        const response = await fetch('/v1/det/single', {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
          },
          body: JSON.stringify({ data: base64Image }),
        });

        if (!response.ok) {
          const errorData = await response.json();
          throw new Error(errorData.error || `HTTP error! status: ${response.status}`);
        }

        const data = await response.json();
        setOutputImage(`data:image/jpeg;base64,${data.output_image}`);
        setDetectionResults(data.results);
      };
      reader.readAsDataURL(selectedFile);
    } catch (err) {
      console.error("检测失败:", err);
      setError(`检测失败: ${err.message}`);
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="App">
      <Container className="my-4">
        <h1 className="text-center mb-4">仪表读数识别系统</h1>
        <Row>
          <Col md={6} className="mb-4">
            <Card>
              <Card.Header>图片上传</Card.Header>
              <Card.Body>
                <Form.Group controlId="formFile" className="mb-3">
                  <Form.Label>选择图片</Form.Label>
                  <Form.Control type="file" accept="image/*" onChange={handleFileChange} />
                </Form.Group>
                {imagePreview && (
                  <div className="mb-3 text-center">
                    <h5>预览</h5>
                    <img src={imagePreview} alt="预览" className="img-fluid" style={{ maxHeight: '300px', maxWidth: '100%' }} />
                  </div>
                )}
                {error && <Alert variant="danger">{error}</Alert>}
                <Button variant="primary" onClick={handleDetect} disabled={loading || !selectedFile} className="w-100">
                  {loading ? (
                    <>
                      <Spinner as="span" animation="border" size="sm" role="status" aria-hidden="true" />
                      <span className="ms-2">检测中...</span>
                    </>
                  ) : (
                    "开始检测"
                  )}
                </Button>
              </Card.Body>
            </Card>
          </Col>
          <Col md={6} className="mb-4">
            <Card>
              <Card.Header>检测结果</Card.Header>
              <Card.Body>
                {outputImage ? (
                  <div className="text-center">
                    <h5>输出图像</h5>
                    <img src={outputImage} alt="检测结果" className="img-fluid mb-3" style={{ maxHeight: '300px', maxWidth: '100%' }} />
                    {detectionResults && detectionResults.length > 0 && (
                      <div>
                        <h5>检测详情:</h5>
                        {detectionResults.map((result, index) => (
                          <div key={index} className="mb-2">
                            <p><strong>类型:</strong> {result.type}</p>
                            <p><strong>读数:</strong> {result.scale_value ? result.scale_value.toFixed(3) + " Mpa" : "N/A"}</p>
                            <p><strong>边界框:</strong> [{result.box.map(coord => coord.toFixed(2)).join(', ')}]</p>
                          </div>
                        ))}
                      </div>
                    )}
                  </div>
                ) : (
                  <p className="text-muted text-center">等待图片上传和检测...</p>
                )}
              </Card.Body>
            </Card>
          </Col>
        </Row>
      </Container>
    </div>
  );
}

export default App;