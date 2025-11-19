<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>PHP APM Elasticsearch Test</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 1200px;
            margin: 50px auto;
            padding: 20px;
            background: #f5f5f5;
        }
        .container {
            background: white;
            padding: 30px;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        h1 {
            color: #333;
            border-bottom: 3px solid #4CAF50;
            padding-bottom: 10px;
        }
        .info {
            background: #e3f2fd;
            padding: 15px;
            border-left: 4px solid #2196F3;
            margin: 20px 0;
        }
        .success {
            background: #e8f5e9;
            padding: 15px;
            border-left: 4px solid #4CAF50;
            margin: 20px 0;
        }
        .warning {
            background: #fff3e0;
            padding: 15px;
            border-left: 4px solid #ff9800;
            margin: 20px 0;
        }
        .error {
            background: #ffebee;
            padding: 15px;
            border-left: 4px solid #f44336;
            margin: 20px 0;
        }
        pre {
            background: #263238;
            color: #aed581;
            padding: 15px;
            border-radius: 4px;
            overflow-x: auto;
        }
        button {
            background: #4CAF50;
            color: white;
            border: none;
            padding: 10px 20px;
            font-size: 16px;
            border-radius: 4px;
            cursor: pointer;
            margin: 5px;
        }
        button:hover {
            background: #45a049;
        }
        .links a {
            display: inline-block;
            margin: 10px 10px 10px 0;
            padding: 10px 20px;
            background: #2196F3;
            color: white;
            text-decoration: none;
            border-radius: 4px;
        }
        .links a:hover {
            background: #1976D2;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🚀 PHP APM with Elasticsearch - Test Page</h1>
        
        <div class="info">
            <strong>PHP Version:</strong> <?php echo phpversion(); ?><br>
            <strong>APM Extension:</strong> <?php echo extension_loaded('apm') ? '✅ Loaded' : '❌ Not Loaded'; ?>
        </div>

        <?php
        // Display APM configuration
        if (extension_loaded('apm')) {
            echo '<div class="success">';
            echo '<h3>APM Configuration:</h3>';
            echo '<pre>';
            $apm_config = array(
                'apm.enabled' => ini_get('apm.enabled'),
                'apm.elasticsearch_enabled' => ini_get('apm.elasticsearch_enabled'),
                'apm.elasticsearch_host' => ini_get('apm.elasticsearch_host'),
                'apm.elasticsearch_port' => ini_get('apm.elasticsearch_port'),
                'apm.elasticsearch_index' => ini_get('apm.elasticsearch_index'),
                'apm.elasticsearch_batch_size' => ini_get('apm.elasticsearch_batch_size'),
                'apm.elasticsearch_batch_timeout' => ini_get('apm.elasticsearch_batch_timeout'),
            );
            foreach ($apm_config as $key => $value) {
                printf("%-35s = %s\n", $key, $value ?: '(not set)');
            }
            echo '</pre>';
            echo '</div>';
        }

        // Test different error levels
        if (isset($_GET['test'])) {
            echo '<h2>📊 Generating Test Events...</h2>';
            
            // Generate some load for stats
            echo '<div class="info"><strong>Simulating workload...</strong></div>';
            $data = array();
            for ($i = 0; $i < 10000; $i++) {
                $data[] = str_repeat('x', 100);
            }
            usleep(100000); // 100ms delay
            
            // Trigger different types of errors
            echo '<div class="warning">';
            echo '<h3>Triggering Test Errors:</h3>';
            
            // Notice
            echo '<p>1. Triggering E_USER_NOTICE...</p>';
            trigger_error("This is a test NOTICE", E_USER_NOTICE);
            
            // Warning
            echo '<p>2. Triggering E_USER_WARNING...</p>';
            trigger_error("This is a test WARNING", E_USER_WARNING);
            
            // Deprecated
            echo '<p>3. Triggering E_USER_DEPRECATED...</p>';
            trigger_error("This is a test DEPRECATED warning", E_USER_DEPRECATED);
            
            echo '</div>';
            
            echo '<div class="success">';
            echo '<p>✅ Test events generated! Check Elasticsearch for data.</p>';
            echo '<p>Documents should appear in the <code>apm-logs</code> index.</p>';
            echo '</div>';
        }
        ?>

        <h2>🎯 Actions</h2>
        <div>
            <a href="?test=1"><button>⚡ Run Test (Generate Events)</button></a>
            <a href="info.php"><button>ℹ️ PHP Info</button></a>
        </div>

        <h2>🔗 Useful Links</h2>
        <div class="links">
            <a href="http://localhost:9200/apm-logs/_search?pretty" target="_blank">📄 View APM Logs (Elasticsearch)</a>
            <a href="http://localhost:9200/_cat/indices?v" target="_blank">📊 Elasticsearch Indices</a>
            <a href="http://localhost:5601" target="_blank">📈 Kibana Dashboard</a>
        </div>

        <h2>📚 Example Queries</h2>
        <div class="info">
            <p><strong>View all APM documents:</strong></p>
            <pre>curl http://localhost:9200/apm-logs/_search?pretty</pre>
            
            <p><strong>View only error events:</strong></p>
            <pre>curl -X POST http://localhost:9200/apm-logs/_search?pretty -H 'Content-Type: application/json' -d '
{
  "query": {
    "term": { "type.keyword": "event" }
  }
}'</pre>
            
            <p><strong>View statistics:</strong></p>
            <pre>curl -X POST http://localhost:9200/apm-logs/_search?pretty -H 'Content-Type: application/json' -d '
{
  "query": {
    "term": { "type.keyword": "stats" }
  }
}'</pre>

            <p><strong>Count documents by type:</strong></p>
            <pre>curl -X POST http://localhost:9200/apm-logs/_search?pretty -H 'Content-Type: application/json' -d '
{
  "size": 0,
  "aggs": {
    "by_type": {
      "terms": { "field": "type.keyword" }
    }
  }
}'</pre>
        </div>

        <h2>🐛 Debugging</h2>
        <div class="warning">
            <p><strong>If events are not appearing in Elasticsearch:</strong></p>
            <ul>
                <li>Check that Elasticsearch is running: <code>curl http://localhost:9200</code></li>
                <li>Verify APM extension is loaded (check above)</li>
                <li>Check Docker logs: <code>docker-compose logs php-app</code></li>
                <li>View network connectivity: <code>docker-compose exec php-app ping elasticsearch</code></li>
            </ul>
        </div>
    </div>
</body>
</html>
