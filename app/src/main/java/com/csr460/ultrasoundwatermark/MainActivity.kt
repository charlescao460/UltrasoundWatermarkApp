package com.csr460.ultrasoundwatermark

import android.Manifest
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.result.contract.ActivityResultContracts
import androidx.activity.viewModels
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.statusBars
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.windowInsetsPadding
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Menu
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.DrawerValue
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.ModalDrawerSheet
import androidx.compose.material3.ModalNavigationDrawer
import androidx.compose.material3.NavigationDrawerItem
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.rememberDrawerState
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import com.csr460.ultrasoundwatermark.ui.CalleeScreen
import com.csr460.ultrasoundwatermark.ui.CallerScreen
import com.csr460.ultrasoundwatermark.ui.MainViewModel
import com.csr460.ultrasoundwatermark.ui.theme.UltrasoundWatermarkTheme
import kotlinx.coroutines.launch

class MainActivity : ComponentActivity() {

    private val viewModel: MainViewModel by viewModels()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()

        val requestPermissionLauncher = registerForActivityResult(ActivityResultContracts.RequestPermission()) {
            // The user has granted or denied the permission.
        }
        requestPermissionLauncher.launch(Manifest.permission.RECORD_AUDIO)

        setContent {
            UltrasoundWatermarkTheme {
                val initState by viewModel.initState.collectAsState()
                Box(modifier = Modifier.fillMaxSize()) {
                    when (val state = initState) {
                        is MainViewModel.InitState.Initializing -> {
                            Box(
                                modifier = Modifier
                                    .fillMaxSize()
                                    .background(Color.Black.copy(alpha = 0.5f)),
                                contentAlignment = Alignment.Center
                            ) {
                                Column(horizontalAlignment = Alignment.CenterHorizontally) {
                                    CircularProgressIndicator()
                                    Spacer(modifier = Modifier.height(16.dp))
                                    Text(text = "Initializing...", style = MaterialTheme.typography.bodyLarge)
                                }
                            }
                        }
                        is MainViewModel.InitState.Ready -> {
                            MainScreen(viewModel)
                        }
                        is MainViewModel.InitState.Error -> {
                            AlertDialog(onDismissRequest = { /*TODO*/ },
                                title = { Text("Error") },
                                text = { Text(state.message) },
                                confirmButton = { TextButton(onClick = { finish() }) { Text("OK") } }
                            )
                        }
                    }
                }
            }
        }
    }
}

@Composable
fun MainScreen(viewModel: MainViewModel) {
    val navController = rememberNavController()
    val drawerState = rememberDrawerState(initialValue = DrawerValue.Closed)
    val scope = rememberCoroutineScope()

    ModalNavigationDrawer(
        drawerState = drawerState,
        drawerContent = {
            ModalDrawerSheet {
                Text("Modes", modifier = Modifier.padding(16.dp), style = MaterialTheme.typography.headlineSmall)
                HorizontalDivider()
                NavigationDrawerItem(
                    label = { Text("Caller") },
                    selected = false,
                    onClick = { 
                        scope.launch {
                            drawerState.close()
                            navController.navigate("caller") 
                        }
                    }
                )
                NavigationDrawerItem(
                    label = { Text("Callee") },
                    selected = false,
                    onClick = { 
                        scope.launch {
                            drawerState.close()
                            navController.navigate("callee")
                        }
                    }
                )
            }
        }
    ) {
        Scaffold(
            topBar = {
                Surface(
                    modifier = Modifier.fillMaxWidth(),
                    shadowElevation = 4.dp,
                    color = MaterialTheme.colorScheme.primary
                ) {
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .windowInsetsPadding(WindowInsets.statusBars)
                            .height(64.dp)
                            .padding(start = 4.dp),
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        IconButton(onClick = { scope.launch { drawerState.open() } }) {
                            Icon(Icons.Default.Menu, contentDescription = "Menu", tint = MaterialTheme.colorScheme.onPrimary)
                        }
                        Spacer(modifier = Modifier.width(12.dp))
                        Text(
                            text = "Ultrasound Watermark",
                            style = MaterialTheme.typography.titleLarge,
                            color = MaterialTheme.colorScheme.onPrimary
                        )
                    }
                }
            }
        ) {
            NavHost(navController, startDestination = "caller", modifier = Modifier.padding(it)) {
                composable("caller") { CallerScreen(viewModel = viewModel) }
                composable("callee") { CalleeScreen(viewModel = viewModel) }
            }
        }
    }
}